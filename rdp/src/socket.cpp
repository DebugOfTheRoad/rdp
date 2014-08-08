#include "socket.h"
#include "test.h"
#include "session.h"
#include "send_buffer.h"
#include "recv_buffer.h"
#include "send.h"
#include "protocol.h"
#include "iocp.h"
#include "epoll.h"
#include "alloc.h"


void socket_recv_result_callback(thread_handle handle, recv_result* result);
void socket_recv_result_timeout(thread_handle handle, const timer_val& tv);

typedef union sessionid {
    RDPSESSIONID sid;
    struct __sid {
        ui64 addr_hash : 48;
        ui64 unused : 15;
        ui64 is_income : 1;
    } _sid;
} sessionid;

static RDPSESSIONID socket_session_make_sessionid(bool income, addrhash addr_hash)
{
    sessionid sid;
    sid._sid.is_income = income;
    sid._sid.unused = 0;
    sid._sid.addr_hash = addr_hash;
    return sid.sid;
}

typedef std::map<addrhash, session*> session_list;

struct session_hash_bucket {
    bool         checkflag;
    mutex_handle lock;
    session_list sess_list;

    session_hash_bucket(){
        checkflag = false;
        lock = 0;
    }
};
struct session_hash {
    ui32                  bucket_size;
    session_hash_bucket*  bucket;
};

typedef struct socket_info_ex : public socket_info {
    //comment begin 下面注视部分数据，运行过程中不会发生变化，不需要使用socket_info锁
    ui32              slot;

    //comment end
    timer_handle      trh; //需要使用socket_info加锁

    //session_hash 使用自身锁，不使用socket_info锁
    session_hash      sess_in;
    session_hash      sess_out;

    bool              checkflag;
} socket_info_ex;

bool socket_session_get_lock_and_addrhash(const session_hash& hash, RDPSESSIONID session_id, mutex_handle& lock_out, ui64& addr_hash);
session* socket_session_get_from(const session_hash& hash, RDPSESSIONID session_id, mutex_handle& lock_out);
void socket_session_insert(const session_hash& hash, session* sess);
bool socket_session_close_from(const session_hash& hash, RDPSESSIONID session_id);

ui32 s_max_udp_dg = 0;
rdp_startup_param s_socket_startup_param = { 0 };
static socket_info_ex s_socket_list[RDPMAXSOCKET] ;


static socket_handle socket_info_ex2socket_handle(socket_info_ex* si)
{
    return (socket_handle)si;
}
static socket_info_ex* socket_handle2socket_info_ex(socket_handle handle)
{
    return (socket_info_ex*)handle;
}
static socket_handle slot2socket_handle(ui16 slot)
{
    if (slot >= s_socket_startup_param.max_sock) {
        return 0;
    }
    return (socket_handle)(slot + 1);
}
static ui16 socket_handle2slot(socket_handle handle)
{
    ui16 slot = ((ui16)handle) - 1;
    return slot;
}
i32 socket_startup(rdp_startup_param* param)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!param) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        if (!param->on_disconnect) {
            ret = RDPERROR_SOCKET_ONDISCONNECTNOTSET;
            break;
        }
        if (!param->on_recv) {
            ret = RDPERROR_SOCKET_ONRECVNOTSET;
            break;
        }
        if (param->version != RDP_SDK_VERSION) {
            ret = RDPERROR_UNKNOWN;
            break;
        }
        if (param->max_sock == 0) {
            param->max_sock = 1;
        }
        if (param->max_sock > _countof(s_socket_list)) {
            param->max_sock = _countof(s_socket_list);
        }
        if (param->recv_thread_num == 0) {
            param->recv_thread_num = 1;
        }
        if (param->recv_buf_size == 0) {
            param->recv_buf_size = 4 * 1024;
        }

        s_socket_startup_param = *param;

        for (ui32 i = 0; i < param->max_sock; ++i) {
            socket_info_ex& so = s_socket_list[i];
            memset(&so, 0, sizeof(socket_info_ex));
            so.slot = i;
            so.state = RDPSOCKETSTATUS_NONE;
            so.sock = INVALID_SOCKET;

            if (i < param->max_sock) {
                so.lock = mutex_create();
                so.cond = 0;// mutex_cond_create();
            }
        }

#ifdef PLATFORM_OS_WINDOWS
        ret = socket_api_startup(s_max_udp_dg);
        if (ret != RDPERROR_SUCCESS) {
            break;
        }
        ret = iocp_create(socket_recv_result_callback, socket_recv_result_timeout);
#else
        ret = epoll_create(socket_recv_result_callback, socket_recv_result_timeout);
#endif
        if (ret != RDPERROR_SUCCESS) {
            break;
        }


    } while (0);
    return ret;
}
i32 socket_cleanup()
{
    for (ui16 i = 0; i < s_socket_startup_param.max_sock; ++i) {
        socket_destroy(slot2socket_handle(i));

        socket_info_ex& so = s_socket_list[i];
        mutex_destroy(so.lock);
        mutex_cond_destroy(so.cond);
        so.lock = 0;
        so.cond = 0;
    }

    return socket_api_cleanup();
}
socket_handle RDPSOCKET2socket_handle(RDPSOCKET sock)
{
    return (socket_handle)sock;
}
RDPSOCKET socket_handle2RDPSOCKET(socket_handle handle)
{
    return (RDPSOCKET)handle;
}
i32 socket_startup_get_param(rdp_startup_param* param)
{
    if (!param) {
        return RDPERROR_INVALIDPARAM;
    }
    *param = s_socket_startup_param;
    return RDPERROR_SUCCESS;
}
i32 socket_create(rdp_socket_create_param* param, socket_handle* handle)
{
    SOCKET sock = INVALID_SOCKET;
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!param ) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }

        if (param->ack_timeout == 0) {
            param->ack_timeout = 300;
        }
        if (param->heart_beat_timeout == 0) {
            param->heart_beat_timeout = 180;
        }
        if (param->in_session_hash_size == 0) {
            param->in_session_hash_size = 1;
        }
        if (param->out_session_hash_size == 0) {
            param->out_session_hash_size = 1;
        }

        sock = socket_api_create(param->v4);
        if (sock == INVALID_SOCKET) {
            ret = RDPERROR_SYSERROR;
            break;
        }

        socket_info_ex* so = 0;
        for (ui16 i = 0; i < s_socket_startup_param.max_sock; ++i) {
            socket_info_ex& s = s_socket_list[i];
            mutex_lock(s.lock);
            if (s.state == RDPSOCKETSTATUS_NONE) {
                s.state = RDPSOCKETSTATUS_INIT;
                so = &s;
            }
            mutex_unlock(s.lock);
            if (so) {
                break;
            }
        }
        if (!so) {
            ret = RDPERROR_SOCKET_RUNOUT;
            break;
        }
        *handle = slot2socket_handle(so->slot);

        so->sock = sock;
        so->addr = socket_api_addr_create(param->v4, 0);

        so->create_param = *param;
        so->sess_in.bucket_size = param->in_session_hash_size;
        so->sess_in.bucket = 0;
        so->sess_out.bucket_size = param->out_session_hash_size;
        so->sess_out.bucket = 0;
        so->checkflag = false;

        if (so->sess_in.bucket_size > 0) {
            so->sess_in.bucket = new session_hash_bucket[so->sess_in.bucket_size];
        }
        if (so->sess_out.bucket_size > 0) {
            so->sess_out.bucket = new session_hash_bucket[so->sess_out.bucket_size] ;
        }
        for (ui32 i = 0; i < so->sess_in.bucket_size; ++i) {
            so->sess_in.bucket[i].lock = mutex_create();
        }
        for (ui32 i = 0; i < so->sess_out.bucket_size; ++i) {
            so->sess_out.bucket[i].lock = mutex_create();
        }
    } while (0);

    if (!(*handle) && sock != INVALID_SOCKET) {
        socket_api_close(sock);
    }
    return ret;
}

i32 socket_get_create_param(socket_handle handle, rdp_socket_create_param* param)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!param) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        *param = so->create_param;
    } while (0);

    return ret;
}
i32 socket_get_state(socket_handle handle, ui32* state)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!state) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        *state = so->state;
    } while (0);

    return ret;
}
i32 socket_destroy(socket_handle handle)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        so->state = RDPSOCKETSTATUS_CLOSING;
        socket_api_close(so->sock);
        so->sock = INVALID_SOCKET;

        timer_destroy(so->trh);
        so->trh = 0;

        session_hash& sess_in = so->sess_in;
        session_hash& sess_out = so->sess_out;

        //加锁
        for (ui32 i = 0; i < sess_in.bucket_size; ++i) {
            session_hash_bucket& bucket = sess_in.bucket[i];
            for (session_list::iterator it = bucket.sess_list.begin();
                    it != bucket.sess_list.end(); ++it) {
                session* sess = it->second;
                session_destroy(sess);
            }
            bucket.sess_list.clear();
        }
        delete []so->sess_in.bucket;
        so->sess_in.bucket = 0;
        sess_in.bucket_size = 0;

        for (ui32 i = 0; i < sess_out.bucket_size; ++i) {
            session_hash_bucket& bucket = sess_out.bucket[i];
            for (session_list::iterator it = bucket.sess_list.begin();
                    it != bucket.sess_list.end(); ++it) {
                session* sess = it->second;
                session_destroy(sess);
            }
            bucket.sess_list.clear();
            mutex_destroy(bucket.lock);
        }
        delete[]so->sess_out.bucket;
        so->sess_out.bucket = 0;
        sess_out.bucket_size = 0;

        so->state = RDPSOCKETSTATUS_NONE;

        mutex_unlock(so->lock);
    } while (0);

    return  ret;
}
i32 socket_bind(socket_handle handle, const char* ip, ui32 port)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        if (so->state == RDPSOCKETSTATUS_NONE) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        if (so->state != RDPSOCKETSTATUS_INIT) {
            ret = RDPERROR_UNKNOWN;
            mutex_unlock(so->lock);
            break;
        }
        ret = socket_api_addr_from(ip, port, so->addr);
        if (ret != RDPERROR_SUCCESS) {
            mutex_unlock(so->lock);
            break;
        }
        bool v4 = socket_api_addr_is_v4(so->addr);
        ret = socket_api_bind(so->sock, so->addr, socket_api_addr_len(so->addr));
        if (ret != 0) {
            ret = RDPERROR_SYSERROR;
            mutex_unlock(so->lock);
            break;
        }
#ifdef PLATFORM_OS_WINDOWS
        ret = iocp_attach(so->sock, v4);
#else
        ret = epoll_attach(so->sock, v4);
#endif
        if (ret != RDPERROR_SUCCESS) {
            break;
        }

        so->state = RDPSOCKETSTATUS_BINDED;

        mutex_unlock(so->lock);
    } while (0);

    return ret;
}
i32 socket_listen(socket_handle handle)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        //检查当前状态
        if (so->state == RDPSOCKETSTATUS_NONE) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        //没有设置监听回调函数
        if (!s_socket_startup_param.on_accept) {
            ret = RDPERROR_SOCKET_ONACCEPTNOTSET;
            mutex_unlock(so->lock);
            break;
        }
        so->state = RDPSOCKETSTATUS_LISTENING;
        mutex_unlock(so->lock);
    } while (0);


    return  ret;
}
i32 socket_connect(socket_handle handle, const char* ip, ui32 port, ui32 timeout,
                   RDPSESSIONID* session_id, const ui8* buf, ui16 buf_len)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!session_id) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        //检查当前状态
        if (so->state == RDPSOCKETSTATUS_NONE) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        //未设置连接回调函数
        if (!s_socket_startup_param.on_connect) {
            ret = RDPERROR_SOCKET_ONCONNECTNOTSET;
            mutex_unlock(so->lock);
            break;
        }
        //转换地址
        sockaddr* addr = socket_api_addr_create(socket_api_addr_is_v4(so->addr), 0);
        bool is_anyaddr = false;
        ret = socket_api_addr_from(ip, port, addr, &is_anyaddr);
        if (ret != RDPERROR_SUCCESS) {
            socket_api_addr_destroy(addr);
            mutex_unlock(so->lock);
            break;
        }
        //拒绝连接 0.0.0.0 和 :: 地址
        if (is_anyaddr) {
            ret = RDPERROR_INVALIDPARAM;
            mutex_unlock(so->lock);
            break;
        }
        //hash 地址
        addrhash addr_hash = socket_api_addr_hash(addr);
        if (addr_hash == 0) {
            ret = RDPERROR_INVALIDPARAM;
            socket_api_addr_destroy(addr);
            mutex_unlock(so->lock);
            break;
        }
        //创建一个传出sessionid
        *session_id = socket_session_make_sessionid(false, addr_hash);
        mutex_handle lock = 0;
        //查找是否已经存在该地址的传出回话
        session* sess = socket_session_get_from(so->sess_out, *session_id, lock);
        if (sess) {
            //该回话已经处于正在连接状态或者已经连接
            if (sess->state == RDPSESSIONSTATUS_CONNECTING ||
                    sess->state == RDPSESSIONSTATUS_CONNECTED) {
                ret = RDPERROR_SESSION_BADSTATE;
                socket_api_addr_destroy(addr);
                mutex_unlock(lock);
                mutex_unlock(so->lock);
                break;
            }
        } else {
            //没有回话,创建一个
            sess = session_create(handle, *session_id, addr);
            socket_session_insert(so->sess_out, sess);
        }
        socket_api_addr_destroy(addr);
        //开始连接
        sess->state = RDPSESSIONSTATUS_CONNECTING;
        ret = session_send_connect(sess, buf, buf_len);

        mutex_unlock(so->lock);
    } while (0);


    return ret;
}

i32 socket_getsyserror()
{
    return socket_api_getsyserror();
}
i32 socket_getsyserrordesc(i32 err, char* desc, ui32 desc_len)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!desc || desc_len == 1) {
            ret =  RDPERROR_INVALIDPARAM;
            break;
        }

        desc[0] = 0;

#if !defined(PLATFORM_OS_WINDOWS)
        strerror_r(err, desc, desc_len);
#else
        LPSTR lpMsgBuf = 0;
        ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL, err,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPSTR)&lpMsgBuf, 0, NULL);
        if (lpMsgBuf) {
            char* dst = desc;
            char* src = lpMsgBuf;
            while ((dst - desc < (i32)desc_len - 1) && *src) {
                *dst++ = *src++;
            }
            *dst = 0;
        }

        ::LocalFree(lpMsgBuf);
#endif
    } while (0);

    return ret;

}
i32 socket_session_close(socket_handle handle, RDPSESSIONID session_id)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (0 == session_id) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        if (so->state == RDPSOCKETSTATUS_NONE) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        bool income = socket_session_is_income(session_id);
        bool b = socket_session_close_from(income ? so->sess_in : so->sess_out, session_id);
        if (!b) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            mutex_unlock(so->lock);
            break;
        }
        mutex_unlock(so->lock);
    } while (0);

    return ret;
}
i32 socket_session_get_state(socket_handle handle, RDPSESSIONID session_id, ui32* state)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!state) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        if (0 == session_id) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        bool income = socket_session_is_income(session_id);
        mutex_handle lock = 0;
        session* sess = socket_session_get_from(income ? so->sess_in : so->sess_out, session_id, lock);
        if (!sess) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            mutex_unlock(lock);
            mutex_unlock(so->lock);
            break;
        }
        *state = sess->state;

        mutex_lock(lock);
        mutex_unlock(so->lock);
    } while (0);

    return ret;
}
i32 socket_session_send(socket_handle handle, RDPSESSIONID session_id, const ui8* buf, ui16 buf_len,
                        ui32 flags,
                        ui32* local_send_queue_size, ui32* peer_unused_recv_queue_size)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (0 == session_id) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        if (so->state == RDPSOCKETSTATUS_NONE ||
                so->state == RDPSOCKETSTATUS_CLOSING) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        if (so->state == RDPSOCKETSTATUS_INIT) {
            ret = RDPERROR_SOCKET_BADSTATE;
            mutex_unlock(so->lock);
            break;
        }
        bool income = socket_session_is_income(session_id);
        mutex_handle lock = 0;
        session* sess = socket_session_get_from(income ? so->sess_in : so->sess_out, session_id, lock);
        if (!sess) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            mutex_unlock(lock);
            mutex_unlock(so->lock);
            break;
        }
        if (sess->state != RDPSESSIONSTATUS_CONNECTED) {
            ret = RDPERROR_SESSION_BADSTATE;
            mutex_unlock(lock);
            mutex_unlock(so->lock);
            break;
        }
        ret = session_send_data(sess, buf, buf_len, flags, local_send_queue_size, peer_unused_recv_queue_size);

        mutex_unlock(lock);
        mutex_unlock(so->lock);
    } while (0);

    return ret == RDPERROR_SUCCESS ? buf_len : ret;
}
bool socket_session_is_income(RDPSESSIONID session_id)
{
    if (0 == session_id) {
        return false;
    }
    sessionid sid;
    sid.sid = session_id;
    return (bool)sid._sid.is_income;
}
i32 socket_udp_send(socket_handle handle, const char* ip, ui32 port, const ui8* buf, ui16 buf_len)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_socket_startup_param.on_udp_recv) {
            ret = RDPERROR_SOCKET_ONUDPRECVNOTSET;
            break;
        }
        if (!buf || buf_len == 0) {
            break;
        }
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(handle);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        SOCKET sock = so->sock;
        if (sock == INVALID_SOCKET) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        //检查状态
        if (so->state == RDPSOCKETSTATUS_NONE ||
                so->state == RDPSOCKETSTATUS_CLOSING) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }
        if (so->state == RDPSOCKETSTATUS_INIT) {
            ret = RDPERROR_SOCKET_BADSTATE;
            mutex_unlock(so->lock);
            break;
        }

        bool v4 = socket_api_addr_is_v4(so->addr);
        mutex_unlock(so->lock);

        sockaddr* addr = socket_api_addr_create(v4, 0);
        ret = socket_api_addr_from(ip, port, addr, 0);
        if (ret != RDPERROR_SUCCESS) {
            socket_api_addr_destroy(addr);
            break;
        }

        protocol_udp_data pack;
        pack.data_size = buf_len;
        protocol_set_header(&pack);

        send_buffer_ex sb;
        memset(&sb, 0, sizeof(send_buffer_ex));
        sb.buf = buffer_create(sizeof(protocol_udp_data)+buf_len);
        memcpy(sb.buf.ptr, &pack, sizeof(protocol_udp_data));
        memcpy(sb.buf.ptr + sizeof(protocol_udp_data), buf, buf_len);
        sb.addr = addr;
        sb.sh = handle;
        sb.sock = so->sock;
        sb.session_id = 0;
        sb.seq_num = 0;
        sb.send_times = 1;

        ret = send_send(&sb);

        buffer_destroy(sb.buf);
        socket_api_addr_destroy(sb.addr);
    } while (0);


    return  ret;
}
i32 socket_addr_to(const sockaddr* addr, ui32 addrlen, char* ip, ui32 iplen, ui32* port)
{
    if (addr->sa_family == AF_INET) {
        if (addrlen != sizeof(sockaddr_in) || iplen < sizeof(sockaddr_in)) {
            return RDPERROR_INVALIDPARAM;
        }
    }
    if (addr->sa_family == AF_INET6 ) {
        if (addrlen != sizeof(sockaddr_in6) || iplen < sizeof(sockaddr_in6)) {
            return RDPERROR_INVALIDPARAM;
        }
    }
    return socket_api_addr_to(addr, ip, iplen, port);
}
socket_info* socket_get_socket_info(SOCKET sock)
{
    for (ui16 i = 0; i < s_socket_startup_param.max_sock; ++i) {
        if (s_socket_list[i].sock == sock) {
            return (socket_info*)&s_socket_list[i];
        }
    }
    return 0;
}
socket_info* socket_get_socket_info(socket_handle handle)
{
    ui16 slot = socket_handle2slot(handle);
    if (slot >= s_socket_startup_param.max_sock) {
        return 0;
    }
    return (socket_info*)&s_socket_list[slot];
}

void socket_recv_result_callback(thread_handle handle, recv_result* result)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        socket_info_ex* so = (socket_info_ex*)socket_get_socket_info(result->sock);
        if (!so) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        mutex_lock(so->lock);
        if (so->state == RDPSOCKETSTATUS_NONE || so->state == RDPSOCKETSTATUS_CLOSING) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            mutex_unlock(so->lock);
            break;
        }

        if (result->buf.length < sizeof(protocol_header)) {
            ret = RDPERROR_UNKNOWN;
            mutex_unlock(so->lock);
            break;
        }

        protocol_header* ph = (protocol_header*)result->buf.ptr;
        //printf("recv msg from %s:%d  protocol:%d  protocol_id:%d  seq_num:%d \n",
        //    ip, port, ph->protocol, (ui32)ph->protocol_id, (ui32)ph->seq_num);

        //检查是否是rdp协议
        if (!protocol_check_header(ph, result->buf.length)) {
            printf("protocol check header failed\n");
            ret = RDPERROR_UNKNOWN;
            mutex_unlock(so->lock);
            break;
        }
        if (ph->protocol >= proto_end) {
            printf("bad protocol id %d\n", ph->protocol);
            ret = RDPERROR_UNKNOWN;
            mutex_unlock(so->lock);
            break;
        }

        if (ph->protocol != proto_udp_data) {
            ui64 addr_hash = socket_api_addr_hash(result->addr);
            if (addr_hash == 0) {
                ret = RDPERROR_UNKNOWN;
                mutex_unlock(so->lock);
                break;
            }

            mutex_handle lock = 0;
            //先查找传入连接
            RDPSESSIONID session_id_out = socket_session_make_sessionid(false, addr_hash);
            RDPSESSIONID session_id_in = socket_session_make_sessionid(true, addr_hash);
            session* sess = socket_session_get_from(so->sess_in, session_id_in, lock);
            if (!sess) {
                //再查找传出连接
                sess = socket_session_get_from(so->sess_out, session_id_out, lock);
            }
            //不存在的会话
            if (!sess) {
                //检查是否是连接请求,如果是,创建新回话
                if (ph->protocol != proto_connect) {
                    ret = RDPERROR_UNKNOWN;
                    mutex_unlock(so->lock);
                    break;
                }
                protocol_connect* p = (protocol_connect*)ph;
                if (result->buf.length != sizeof(protocol_connect)+p->data_size) {
                    ret = RDPERROR_UNKNOWN;
                    mutex_unlock(so->lock);
                    break;
                }
                //检查当前socket是否处于监听状态
                if (so->state != RDPSOCKETSTATUS_LISTENING) {
                    ret = RDPERROR_UNKNOWN;
                    mutex_unlock(so->lock);
                    break;
                }
                rdp_on_before_accept param;
                param.sock = so->sock;
                param.session_id = session_id_in;
                param.addr = result->addr;
                param.addrlen = socket_api_addr_len(result->addr);
                param.buf = p->data_size != 0 ? (ui8*)(p + 1) : 0;
                param.buf_len = p->data_size;
                //过滤连接请求
                if (s_socket_startup_param.on_before_accept) {
                    if (!s_socket_startup_param.on_before_accept(param)) {
                        mutex_unlock(so->lock);
                        break;
                    }
                }
                //创建新连接
                sess = session_create(slot2socket_handle(so->slot), session_id_in, result->addr);
                sess->state = RDPSESSIONSTATUS_CONNECTED;
                socket_session_insert(so->sess_in, sess);
                ui64 addr_hash0 = 0;
                socket_session_get_lock_and_addrhash(so->sess_in, session_id_in, lock, addr_hash0);
                mutex_lock(lock);
                //投递accept给上层
                s_socket_startup_param.on_accept(param);
            }
            //无效回话
            if (!sess) {
                ret = RDPERROR_UNKNOWN;
                mutex_unlock(lock);
                mutex_unlock(so->lock);
                break;
            }
            session_handle_recv(sess, result);
            if (sess->state == RDPSESSIONSTATUS_DISCONNECTING) {
                bool income = socket_session_is_income(sess->session_id);
                socket_session_close_from(income ? so->sess_in : so->sess_out, sess->session_id);
            }
            mutex_unlock(lock);
        } else {
            protocol_udp_data* p = (protocol_udp_data*)ph;

            rdp_on_udp_recv_param param;

            param.sock = socket_handle2RDPSOCKET(socket_info_ex2socket_handle(so));
            param.addr = result->addr;
            param.addrlen = socket_api_addr_len(result->addr);
            param.buf = p->data_size != 0 ? (ui8*)(++p) : 0;
            param.buf_len = p->data_size;

            s_socket_startup_param.on_udp_recv(param);
        }

        mutex_unlock(so->lock);
    } while (0);


    if (ret != RDPERROR_SUCCESS) {

    }
}
void socket_recv_result_timeout(thread_handle handle, const timer_val& tv)
{
    ui64 addr_hash = 0;
    socket_info_ex* so = 0;
    for (ui16 i = 0; i < s_socket_startup_param.max_sock; ++i) {
        socket_info_ex& s = s_socket_list[i];
        if (s.state != RDPSOCKETSTATUS_BINDED &&
            s.state != RDPSOCKETSTATUS_LISTENING) {
            break;
        }
        if (s.checkflag){
            continue;
        }
        session_hash& sess_in = s.sess_in;
        session_hash& sess_out = s.sess_out;
        for (ui32 i = 0; i < sess_in.bucket_size; ++i) {
            session_hash_bucket& bucket = sess_in.bucket[i];
            if (bucket.checkflag) {
                continue;
            }
            if (mutex_trylock(bucket.lock)) {
                session_list::iterator it = bucket.sess_list.begin();
                session_list::iterator end = bucket.sess_list.end();

                for (; it != end; ++it) {
                    session* sess = it->second;
                    session_send_check(sess, tv);
                }
                mutex_unlock(bucket.lock);
            }
        }
    }

}
bool socket_session_get_lock_and_addrhash(const session_hash& hash, RDPSESSIONID session_id, mutex_handle& lock_out, ui64& addr_hash)
{
    if (session_id == 0) {
        return false;
    }
    sessionid sid;
    sid.sid = session_id;
    addr_hash = sid._sid.addr_hash;
    i32 lock_index = addr_hash % hash.bucket_size;
    hash.bucket[lock_index].lock;
    return true;
}
session* socket_session_get_from(const session_hash& hash, RDPSESSIONID session_id, mutex_handle& lock_out)
{
    mutex_handle lock = 0;
    ui64 addr_hash = 0;
    if (!socket_session_get_lock_and_addrhash(hash, session_id, lock, addr_hash)) {
        return 0;
    }

    i32 sess_index = addr_hash % hash.bucket_size;
    session_hash_bucket& bucket = hash.bucket[sess_index];

    mutex_lock(lock);
    if (bucket.sess_list.size() == 0) {
        mutex_unlock(lock);
        return 0;
    }
    session_list::iterator it = bucket.sess_list.find(addr_hash);
    if (it == bucket.sess_list.end()) {
        mutex_unlock(lock);
        return 0;
    }
    lock_out = lock;
    return it->second;
}
void socket_session_insert(const session_hash& hash, session* sess)
{
    mutex_handle lock = 0;
    ui64 addr_hash = 0;
    if (!socket_session_get_lock_and_addrhash(hash, sess->session_id, lock, addr_hash)) {
        return ;
    }

    i32 sess_index = addr_hash % hash.bucket_size;
    session_hash_bucket& bucket = hash.bucket[sess_index];

    mutex_lock(lock);
    bucket.sess_list[addr_hash] = sess;
    mutex_unlock(lock);
}
bool socket_session_close_from(const session_hash& hash, RDPSESSIONID session_id)
{
    mutex_handle lock = 0;
    ui64 addr_hash = 0;
    if (!socket_session_get_lock_and_addrhash(hash, session_id, lock, addr_hash)) {
        return false;
    }

    i32 sess_index = addr_hash % hash.bucket_size;
    session_hash_bucket& bucket = hash.bucket[sess_index];

    mutex_lock(lock);
    session_list::iterator it = bucket.sess_list.find(addr_hash);
    if (it == bucket.sess_list.end()) {
        mutex_unlock(lock);
        return false;
    }
    session_send_disconnect(it->second, 0);

    bucket.sess_list.erase(it);
    session_destroy(it->second);

    mutex_unlock(lock);
    return true;
}


#ifdef PLATFORM_CONFIG_TEST
void socket_test()
{

}
#endif