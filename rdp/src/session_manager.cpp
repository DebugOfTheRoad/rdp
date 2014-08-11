#include "session_manager.h"
#include "thread.h"
#include "send_buffer.h"
#include "recv_buffer.h"
#include "protocol.h"
#include "alloc.h"
#include "socket.h"
#include "session.h"
#include <set>



SessionManager::SessionManager()
{
    in_come_id_ = 1;
    out_come_id_ = 1;
    last_in_come_update_hash_id_ = 0;
    last_out_come_update_hash_id_ = 0;
    memset(&last_update_, 0, sizeof(last_update_));
}
SessionManager::~SessionManager()
{

}
void SessionManager::create(Socket* socket, ui16 in_come_hash_size)
{
    socket_ = socket;
    in_come_session_id_list_.create(in_come_hash_size, hash_key_id_ui32, hash_key_cmp_ui32);
    in_come_session_addr_list_.create(in_come_hash_size, hash_key_id_sockaddr, hash_key_cmp_sockaddr);

    out_come_session_id_list_.create(1, hash_key_id_ui32, hash_key_cmp_ui32);
    out_come_session_addr_list_.create(1, hash_key_id_sockaddr, hash_key_cmp_sockaddr);
}
void SessionManager::destroy()
{
    for (ui32 i = 0; i < in_come_session_id_list_.size(); ++i) {
        SessionIdList::Bucket* bucket = in_come_session_id_list_.get_at(i);
        while (bucket) {
            Session* sess = bucket->value_;
            bucket = bucket->next_;

            sess->disconnect(0);
            sess->destroy();
            alloc_delete_object(sess);
        }
    }
    for (ui32 i = 0; i < out_come_session_id_list_.size(); ++i) {
        SessionIdList::Bucket* bucket = out_come_session_id_list_.get_at(i);
        while (bucket) {
            Session* sess = bucket->value_;
            bucket = bucket->next_;

            sess->disconnect(0);
            sess->destroy();
            alloc_delete_object(sess);
        }
    }

    socket_ = 0;
    in_come_session_id_list_.destroy();
    in_come_session_addr_list_.destroy();
    out_come_session_id_list_.destroy();
    out_come_session_addr_list_.destroy();

    in_come_id_ = 1;
    out_come_id_ = 1;
    last_in_come_update_hash_id_ = 0;
    last_out_come_update_hash_id_ = 0;
    memset(&last_update_, 0, sizeof(last_update_));
}
Session* SessionManager::find(RDPSESSIONID session_id)
{
    sessionid sid;
    sid.sid = session_id;

    Session* sess = 0;

    do {
        if (sid.sid == 0) {
            break;
        }
        if (sid._sid.is_in_come) {
            in_come_session_id_list_.find(sid._sid.id, sess);
        } else {
            out_come_session_id_list_.find(sid._sid.id, sess);
        }
    } while (0);

    return sess;
}
Session* SessionManager::find(const sockaddr* addr)
{
    Session* sess = 0;

    addr_key addrkey;
    addrkey.addr = addr;

    if (out_come_session_addr_list_.find(addrkey, sess)) {
        return sess;
    }

    in_come_session_addr_list_.find(addrkey, sess);

    return sess;
}

i32 SessionManager::connect(const char* ip, ui32 port, ui32 timeout, const ui8* buf, ui16 buf_len, RDPSESSIONID* session_id)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        sockaddr* addr = socket_api_addr_create(socket_->get_addr());
        bool is_anyaddr = false;
        ret = socket_api_addr_from(ip, port, addr, &is_anyaddr);
        if (ret != RDPERROR_SUCCESS) {
            socket_api_addr_destroy(addr);
            break;
        }
        if (is_anyaddr) {
            ret = RDPERROR_INVALIDPARAM;
            socket_api_addr_destroy(addr);
            break;
        }

        Session* sess = find(addr);
        if (!sess) {
            if (in_come_id_ >= 0xffffffff) {
                in_come_id_ = 1;
            }

            sessionid sid;
            sid._sid.socket_slot = socket_->get_slot();
            sid._sid.is_v4 = socket_->get_create_param().is_v4;
            sid._sid.is_in_come = false;
            sid._sid.unused = 0;
            sid._sid.id = out_come_id_++;

            sess = alloc_new_object<Session>();
            sess->create(this, sid.sid, addr);

            addr_key addrkey;
            addrkey.addr = sess->get_addr();

            out_come_session_id_list_.insert(sid._sid.id, sess);
            out_come_session_addr_list_.insert(addrkey, sess);
        }
        *session_id = sess->get_session_id();
        ret = sess->connect(timeout, buf, buf_len);

        socket_api_addr_destroy(addr);
    } while (0);

    return ret;
}
i32 SessionManager::close(RDPSESSIONID session_id, ui16 reason, bool send_disconnect)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        sessionid sid;
        sid.sid = session_id;

        Session* sess = 0;

        if (sid._sid.is_in_come) {
            if (in_come_session_id_list_.remove(sid._sid.id, sess)) {
                addr_key addrkey;
                addrkey.addr = sess->get_addr();
                in_come_session_addr_list_.remove(addrkey);
            }
        } else {
            if (out_come_session_id_list_.remove(sid._sid.id, sess)) {
                addr_key addrkey;
                addrkey.addr = sess->get_addr();
                out_come_session_addr_list_.remove(addrkey);
            }
        }

        if (!sess) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            break;
        }

        sess->disconnect(reason);
        sess->destroy();
        alloc_delete_object(sess);
    } while (0);

    return ret;
}
i32 SessionManager::get_state(RDPSESSIONID session_id, ui32*state)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        Session* sess = find(session_id);
        if (!sess) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            break;
        }
        *state = sess->get_state();
    } while (0);

    return ret;
}
i32 SessionManager::send(RDPSESSIONID session_id, const ui8* buf, ui16 buf_len, ui32 flags)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        Session* sess = find(session_id);
        if (!sess) {
            ret = RDPERROR_SESSION_INVALIDSESSIONID;
            break;
        }
        ret = sess->send(buf, buf_len, flags);
    } while (0);
    return ret;
}
i32 SessionManager::udp_send(sockaddr* addr, const ui8* buf, ui16 buf_len)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        protocol_udp_data pack;
        pack.data_size = buf_len;
        protocol_set_header(&pack);

        send_buffer sb;
        memset(&sb, 0, sizeof(send_buffer));
        sb.buf = buffer_create(sizeof(protocol_udp_data)+buf_len);
        memcpy(sb.buf.ptr, &pack, sizeof(protocol_udp_data));
        memcpy(sb.buf.ptr + sizeof(protocol_udp_data), buf, buf_len);
        sb.addr = addr;
        sb.socket = socket_->get_socket();
        sb.session_id = 0;
        sb.seq_num = 0;

        ret = socket_->send(&sb);

        buffer_destroy(sb.buf);
    } while (0);

    return ret;
}
void SessionManager::on_recv(thread_handle handle, recv_result* result)
{
    do {
        if (result->buf.length == 0) {
            Session* sess = find(result->addr);
            if (sess) {
                protocol_disconnect pack;
                pack.reason = 0;

                sess->on_recv(&pack);
                close(sess->get_session_id(), 0, false);
            }
            break;
        }
        if (result->buf.length < sizeof(protocol_header)) {
            break;
        }

        protocol_header* ph = (protocol_header*)result->buf.ptr;
        //printf("recv msg from %s:%d  protocol:%d  protocol_id:%d  seq_num:%d \n",
        //    ip, port, ph->protocol, (ui32)ph->protocol_id, (ui32)ph->seq_num);

        //检查是否是rdp协议
        if (!protocol_check_header(ph, result->buf.length)) {
            //printf("protocol check protocol header failed\n");
            break;
        }
        const rdp_startup_param& sparam = socket_get_startup_param();

        if (ph->protocol == proto_udp_data) {
            if (sparam.on_udp_recv) {
                protocol_udp_data* p = (protocol_udp_data*)ph;

                rdp_on_udp_recv_param param;

                param.sock = socket_->get_rdpsocket();
                param.addr = result->addr;
                param.addrlen = socket_api_addr_len(result->addr);
                param.buf = p->data_size != 0 ? (ui8*)(++p) : 0;
                param.buf_len = p->data_size;

                sparam.on_udp_recv(&param);
            }

            break;
        }

        Session* sess = find(result->addr);
        if (!sess) {
            if (ph->protocol != proto_connect) {
                break;
            }
            protocol_connect* p = (protocol_connect*)ph;

            //检查当前socket是否处于监听状态
            if (socket_->get_state() != RDPSOCKETSTATUS_LISTENING) {
                break;
            }
            if (in_come_id_ >= 0xffffffff) {
                in_come_id_ = 1;
            }
            sessionid sid;
            sid._sid.socket_slot = socket_->get_slot();
            sid._sid.is_v4 = socket_->get_create_param().is_v4;
            sid._sid.is_in_come = true;
            sid._sid.unused = 0;
            sid._sid.id = in_come_id_++;

            rdp_on_before_accept param;
            param.sock = socket_->get_rdpsocket();
            param.session_id = sid.sid;
            param.addr = result->addr;
            param.addrlen = socket_api_addr_len(result->addr);
            param.buf = p->data_size != 0 ? (ui8*)(p + 1) : 0;
            param.buf_len = p->data_size;
            //过滤连接请求
            if (sparam.on_before_accept) {
                if (!sparam.on_before_accept(&param)) {
                    break;
                }
            }
            //创建新连接
            sess = alloc_new_object<Session>();
            sess->create(this, sid.sid, result->addr);

            addr_key addrkey;
            addrkey.addr = sess->get_addr();

            in_come_session_id_list_.insert(sid._sid.id, sess);
            in_come_session_addr_list_.insert(addrkey, sess);

            //投递accept给上层
            sparam.on_accept(&param);
        }

        //无效回话
        if (!sess) {
            break;
        }

        sess->on_recv(ph);
        if (sess->get_state() == RDPSESSIONSTATUS_INIT) {
            close(sess->get_session_id(), 0, false);
        }
    } while (0);
    on_update(handle, result->now);
}
void SessionManager::on_update(thread_handle handle, const timer_val& now)
{
    if (timer_is_empty(last_update_)) {
        last_update_ = now;
        return;
    }
    if (timer_sub_msec(now, last_update_) < 10) {
        return;
    }
    last_update_ = now;

    if (last_in_come_update_hash_id_ >= in_come_session_id_list_.size()) {
        last_in_come_update_hash_id_ = 0;
    }
    if (last_out_come_update_hash_id_ >= out_come_session_id_list_.size()) {
        last_out_come_update_hash_id_ = 0;
    }

    std::set<Session*> to_removes;

    SessionIdList::Bucket* in_come_bucket = in_come_session_id_list_.get_at(last_in_come_update_hash_id_);
    while (in_come_bucket) {
        Session* sess = in_come_bucket->value_;
        sess->on_update(now);
        if (sess->get_state() == RDPSESSIONSTATUS_INIT) {
            to_removes.insert(sess);
        }
        in_come_bucket = in_come_bucket->next_;
    }
    if (!to_removes.empty()) {
        addr_key addrkey;
        for (std::set<Session*>::iterator it = to_removes.begin();
                it != to_removes.end(); ++it) {
            Session* sess = *it;

            sessionid sid;
            sid.sid = sess->get_session_id();
            addrkey.addr = sess->get_addr();

            in_come_session_id_list_.remove(sid._sid.id);
            in_come_session_addr_list_.remove(addrkey);

            sess->destroy();
            alloc_delete_object(sess);
        }
        to_removes.clear();
    }

    SessionIdList::Bucket* out_come_bucket = out_come_session_id_list_.get_at(last_out_come_update_hash_id_);
    while (out_come_bucket) {
        Session* sess = out_come_bucket->value_;
        sess->on_update(now);
        if (sess->get_state() == RDPSESSIONSTATUS_INIT) {
            to_removes.insert(sess);
        }
        out_come_bucket = out_come_bucket->next_;
    }
    if (!to_removes.empty()) {
        addr_key addrkey;
        for (std::set<Session*>::iterator it = to_removes.begin();
                it != to_removes.end(); ++it) {
            Session* sess = *it;

            sessionid sid;
            sid.sid = sess->get_session_id();
            addrkey.addr = sess->get_addr();

            out_come_session_id_list_.remove(sid._sid.id);
            out_come_session_addr_list_.remove(addrkey);

            sess->destroy();
            alloc_delete_object(sess);
        }
        to_removes.clear();
    }

    ++last_in_come_update_hash_id_;
    ++last_out_come_update_hash_id_;
}


