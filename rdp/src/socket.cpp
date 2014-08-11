#include "socket.h"
#include "test.h"
#include "Session.h"
#include "send_buffer.h"
#include "recv_buffer.h"
#include "protocol.h"
#include "iocp.h"
#include "epoll.h"
#include "alloc.h"
#include "session_manager.h"
#include <map>


//////////////////////////////////////////////////////////////////////////

void socket_recv_result_callback(thread_handle handle, recv_result* result);
void socket_recv_result_timeout_callback(thread_handle handle, const timer_val& now);

//////////////////////////////////////////////////////////////////////////
#define SOCKET_SLOT_SEED 0x37

ui32              s_max_udp_dg = 0;
rdp_startup_param s_startup_param = { 0 };
static Socket     s_socket_list[256];

inline RDPSOCKET slot2RDPSOCKET(ui16 slot)
{
    return (slot + 1) ^ SOCKET_SLOT_SEED;
}
inline ui16 RDPSOCKET2slot(RDPSOCKET sock)
{
    sock ^= SOCKET_SLOT_SEED;
    return sock - 1;
}
//////////////////////////////////////////////////////////////////////////
Socket::Socket()
{
    slot_ = 0;
    state_ = RDPSOCKETSTATUS_NONE;
    socket_ = INVALID_SOCKET;
    addr_ = 0;
}
Socket::~Socket()
{

}
void Socket::startup(ui8 slot)
{
    slot_ = slot;
    lock_ = mutex_create();
    addr_ = (sockaddr*)alloc_new(sizeof(sockaddr_in6));
    memset(addr_, 0, sizeof(sockaddr_in6));
    sm_ = new SessionManager;
}
void Socket::cleanup()
{
    delete sm_;
    sm_ = 0;
    alloc_delete(addr_);
    addr_ = 0;
    mutex_destroy(lock_);
    lock_ = 0;
}
i32 Socket::create(rdp_socket_create_param* param)
{
    i32 ret = RDPERROR_SUCCESS;

    if (param->ack_timeout == 0) {
        param->ack_timeout = 300;
    }
    if (param->heart_beat_timeout == 0) {
        param->heart_beat_timeout = 180;
    }
    if (param->in_session_hash_size == 0) {
        param->in_session_hash_size = 1;
    }

    do {
        socket_ = socket_api_create(param->is_v4);
        if (socket_ == INVALID_SOCKET) {
            ret = RDPERROR_SYSERROR;
            break;
        }
        create_param_ = *param;
        state_ = RDPSOCKETSTATUS_INIT;
        addr_->sa_family = create_param_.is_v4 ? AF_INET : AF_INET6;

        sm_->create(this, create_param_.in_session_hash_size);
    } while (0);

    return ret;
}
void Socket::destroy()
{
#ifdef PLATFORM_OS_WINDOWS
    iocp_detach(socket_);
#else
    epoll_detach(socket_);
#endif
    socket_api_close(socket_);
    state_ = RDPSOCKETSTATUS_NONE;
    socket_ = INVALID_SOCKET;
    memset(addr_, 0, sizeof(sockaddr_in6));
    sm_->destroy();
}
RDPSOCKET Socket::get_rdpsocket()
{
    return slot2RDPSOCKET(slot_);
}
i32 Socket::bind(const char* ip, ui32 port)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (state_ != RDPSOCKETSTATUS_INIT) {
            ret = RDPERROR_SOCKET_BADSTATE;
            break;
        }
        ret = socket_api_addr_from(ip, port, addr_);
        if (ret != RDPERROR_SUCCESS) {
            break;
        }
        ret = socket_api_bind(socket_, addr_, socket_api_addr_len(addr_));
        if (ret != 0) {
            ret = RDPERROR_SYSERROR;
            break;
        }
#ifdef PLATFORM_OS_WINDOWS
        ret = iocp_attach(slot_, socket_, create_param_.is_v4);
#else
        ret = epoll_attach(slot_, socket_, create_param_.is_v4);
#endif
        if (ret != RDPERROR_SUCCESS) {
            break;
        }

        state_ = RDPSOCKETSTATUS_BINDED;
    } while (0);
    return ret;
}
i32 Socket::listen()
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        //检查当前状态
        if (state_ != RDPSOCKETSTATUS_BINDED) {
            ret = RDPERROR_SOCKET_BADSTATE;
            break;
        }
        state_ = RDPSOCKETSTATUS_LISTENING;
    } while (0);
    return ret;
}
i32 Socket::connect(const char* ip, ui32 port, ui32 timeout, const ui8* buf, ui16 buf_len, RDPSESSIONID* session_id)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        //检查当前状态
        if (state_ != RDPSOCKETSTATUS_BINDED &&
                state_ != RDPSOCKETSTATUS_LISTENING) {
            ret = RDPERROR_SOCKET_BADSTATE;
            break;
        }
        ret = sm_->connect(ip, port, timeout, buf, buf_len, session_id);
    } while (0);
    return ret;
}
i32 Socket::session_close(RDPSESSIONID session_id, i32 reason)
{
    return sm_->close(session_id, reason);
}
i32 Socket::session_get_state(RDPSESSIONID session_id, ui32* state)
{
    return sm_->get_state(session_id, state);
}
i32 Socket::session_send(RDPSESSIONID session_id, const ui8* buf, ui16 buf_len, ui32 flags)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (state_ != RDPSOCKETSTATUS_BINDED &&
                state_ != RDPSOCKETSTATUS_LISTENING) {
            ret = RDPERROR_SOCKET_BADSTATE;
            break;
        }
        ret = sm_->send(session_id, buf, buf_len, flags);
    } while (0);
    return ret;
}
i32 Socket::udp_send(const char* ip, ui32 port, const ui8* buf, ui16 buf_len)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (state_ != RDPSOCKETSTATUS_BINDED &&
                state_ != RDPSOCKETSTATUS_LISTENING) {
            ret = RDPERROR_SOCKET_BADSTATE;
            break;
        }

        sockaddr_in6 addr = { 0 };
        sockaddr* addr_in = (sockaddr*)&addr;
        addr_in->sa_family = create_param_.is_v4 ? AF_INET : AF_INET6;
        ret = socket_api_addr_from(ip, port, (sockaddr*)&addr, 0);
        if (ret != RDPERROR_SUCCESS) {
            break;
        }
        ret = sm_->udp_send(addr_in, buf, buf_len);
    } while (0);

    return ret;
}
void Socket::on_recv(thread_handle handle, recv_result* result)
{
    do {
        if (result->addr->sa_family == AF_INET) {
            sockaddr_in* addr = (sockaddr_in*)result->addr;
            if (addr->sin_port == 0) {
                break;
            }
        } else {
            sockaddr_in6* addr = (sockaddr_in6*)result->addr;
            if (addr->sin6_port == 0) {
                break;
            }
        }
        sm_->on_recv(handle, result);
    } while (0);
}
void Socket::on_update(thread_handle handle, const timer_val& now)
{
    sm_->on_update(handle, now);
}
i32 Socket::send(send_buffer* send_buf)
{
    send_buf->socket = socket_;
    send_buf->send_time = timer_get_current_time();
    i32 ret = socket_api_sendto(send_buf->socket,
                                send_buf->buf,
                                0,
                                send_buf->addr,
                                socket_api_addr_len(send_buf->addr));

    if (ret < 0) {
        return RDPERROR_SYSERROR;
    }
    return ret;
}
//////////////////////////////////////////////////////////////////////////
const rdp_startup_param& socket_get_startup_param()
{
    return s_startup_param;
}
Socket* socket_get_from_slot_socket(ui8 slot, SOCKET sock, mutex_handle& lock)
{
    if (slot >= s_startup_param.max_sock){
        return 0;
    }
   
    Socket& socket = s_socket_list[slot];
    mutex_handle lo = socket.get_lock();
    mutex_lock(lo);
    if (socket.get_socket() == sock) {
        lock = lo;
        return &socket;
    }
    mutex_unlock(lo);
     
    return 0;
}
Socket* socket_get_from_rdpsocket(RDPSOCKET sock, mutex_handle& lock)
{
    ui32 slot = RDPSOCKET2slot(sock);
    if (slot >= s_startup_param.max_sock) {
        return 0;
    }
    Socket& socket = s_socket_list[slot];
    mutex_handle lo = socket.get_lock();
    mutex_lock(lo);
    if (socket.get_state() == RDPSOCKETSTATUS_NONE ||
        socket.get_socket() == INVALID_SOCKET) {
        mutex_unlock(lo);
        return 0;
    }

    lock = lo;
    return &socket;
}

i32 socket_startup(rdp_startup_param* param)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
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
            param->max_sock = (ui8)_countof(s_socket_list);
        }
        if (param->recv_thread_num == 0) {
            param->recv_thread_num = 1;
        }
        if (param->recv_buf_size == 0) {
            param->recv_buf_size = 4 * 1024;
        }

        s_startup_param = *param;

        ret = socket_api_startup(s_max_udp_dg);
        if (ret != RDPERROR_SUCCESS) {
            break;
        }

        for (ui32 i = 0; i < s_startup_param.max_sock; ++i) {
            s_socket_list[i].startup(i);
        }

#ifdef PLATFORM_OS_WINDOWS
        ret = iocp_create(socket_recv_result_callback, socket_recv_result_timeout_callback);
#else
        ret = epoll_create(socket_recv_result_callback, socket_recv_result_timeout_callback);
#endif
    } while (0);
    return ret;
}
i32 socket_cleanup()
{
#ifdef PLATFORM_OS_WINDOWS
    iocp_destroy();
#else
    epoll_destroy();
#endif

    for (ui16 i = 0; i < s_startup_param.max_sock; ++i) {
        Socket& socket = s_socket_list[i];
        mutex_handle lock = socket.get_lock();
        mutex_lock(lock);
        socket.destroy();
        mutex_unlock(lock);
        socket.cleanup();
    }

    return socket_api_cleanup();
}
i32 socket_startup_get_param(rdp_startup_param* param)
{
    *param = s_startup_param;
    return RDPERROR_SUCCESS;
}
i32 socket_create(rdp_socket_create_param* param, Socket** socket, mutex_handle& lock_out)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        Socket* so = 0;
        for (ui16 i = 0; i < s_startup_param.max_sock; ++i) {
            Socket& sock = s_socket_list[i];
            mutex_handle lock = sock.get_lock();
            mutex_lock(lock);

            if (sock.get_state() == RDPSOCKETSTATUS_NONE) {
                ret = sock.create(param);
                if (ret != RDPERROR_SUCCESS) {
                    sock.destroy();
                    mutex_unlock(lock);
                    break;
                }
                so = &sock;
                lock_out = lock;
                break;
            }
            mutex_unlock(lock);
        }
        if (!so) {
            ret = RDPERROR_SOCKET_RUNOUT;
            break;
        }

        *socket = so;

    } while (0);

    return ret;
}

void socket_recv_result_callback(thread_handle handle, recv_result* result)
{
    do {
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_slot_socket(result->slot, result->sock, lock);
        if (!socket) {
            break;
        }
        socket->on_recv(handle, result);
        mutex_unlock(lock);
    } while (0);
}
void socket_recv_result_timeout_callback(thread_handle handle, const timer_val& now)
{
    for (ui8 i = 0; i < s_startup_param.max_sock; ++i) {
        Socket& socket = s_socket_list[i];
        mutex_handle lock = socket.get_lock();
        if (mutex_trylock(lock)) {
            if (socket.get_socket() != INVALID_SOCKET) {
                socket.on_update(handle, now);
            }
            mutex_unlock(lock);
        }
    }
}




#ifdef PLATFORM_CONFIG_TEST
void socket_test()
{

}
#endif