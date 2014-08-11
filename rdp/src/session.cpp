#include "Session.h"
#include "thread.h"
#include "send_buffer.h"
#include "recv_buffer.h"
#include "protocol.h"
#include "alloc.h"
#include "socket.h"
#include "session_manager.h"

Session::Session()
{
    manager_ = 0;
    session_id_.sid = 0;
    state_ = RDPSESSIONSTATUS_NONE;
    memset(&addr_, 0, sizeof(sockaddr_in6));
    seq_num_ = 1;
    memset(&recv_last_, 0, sizeof(recv_last_));
    peer_window_size_ = 1024;
    peer_ack_timerout_ = 128;
}
Session::~Session()
{

}
void Session::create(SessionManager* manager, RDPSESSIONID session_id, const sockaddr* addr)
{
    manager_ = manager;
    session_id_.sid = session_id;
    state_ = RDPSESSIONSTATUS_INIT;

    if (addr->sa_family == AF_INET) {
        memcpy(&addr_, addr, sizeof(sockaddr_in));
    }
    else {
        memcpy(&addr_, addr, sizeof(sockaddr_in6));
    }
}
void Session::destroy()
{
    for (send_buffer_list::iterator it = send_buffer_list_.begin();
            it != send_buffer_list_.end(); ++it) {
        send_buffer* sb = it->second;
        buffer_destroy(sb->buf);
        alloc_delete_object(sb);
    }
    send_buffer_list_.clear();

    seq_num_ = 1;
    memset(&recv_last_, 0, sizeof(recv_last_));
    peer_window_size_ = 1024;
    peer_ack_timerout_ = 128;
    state_ = RDPSESSIONSTATUS_INIT;
    session_id_.sid = 0;
    memset(&addr_, 0, sizeof(sockaddr_in6));
    manager_ = 0;
}
i32 Session::ctrl(ui16 cmd)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (seq_num_ >= 0xffffffff) {
            seq_num_ = 1;
        }
        ui32 seq_num = seq_num_++;

        protocol_ctrl pack;
        pack.seq_num = seq_num;
        pack.cmd = cmd;
        protocol_set_header(&pack);

        send_buffer* sb = alloc_new_object<send_buffer>();
        memset(sb, 0, sizeof(send_buffer));
        sb->buf = buffer_create((const ui8*)&pack, sizeof(protocol_ctrl));
        sb->addr = get_addr();
        sb->session_id = session_id_.sid;
        sb->seq_num = seq_num;
        sb->peer_ack_timerout = peer_ack_timerout_;
        sb->first_send_time = timer_get_current_time();

        ret = manager_->get_socket()->send(sb);
        if (sb->buf.length == ret) {
            send_buffer_list_[sb->seq_num] = sb;
        } else {
            buffer_destroy(sb->buf);
            alloc_delete_object(sb);
        }
    } while (0);
    return ret;
}
i32 Session::connect(ui32 timeout, const ui8* buf, ui16 buf_len)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (state_ == RDPSESSIONSTATUS_CONNECTING ||
                state_ == RDPSESSIONSTATUS_CONNECTED) {
            ret = RDPERROR_SESSION_BADSTATE;
            break;
        }
        state_ = RDPSESSIONSTATUS_CONNECTING;

        if (seq_num_ >= 0xffffffff) {
            seq_num_ = 1;
        }
        ui32 seq_num = seq_num_++;

        protocol_connect pack;
        pack.seq_num = seq_num;
        pack.ack_timeout = 300;
        pack.connect_timeout = timeout;
        pack.data_size = buf_len;
        protocol_set_header(&pack);

        send_buffer* sb = alloc_new_object<send_buffer>();
        memset(sb, 0, sizeof(send_buffer));
        sb->buf = buffer_create(sizeof(protocol_connect)+buf_len);
        memcpy(sb->buf.ptr, &pack, sizeof(protocol_connect));
        memcpy(sb->buf.ptr + sizeof(protocol_connect), buf, buf_len);
        sb->addr = get_addr();
        sb->session_id = session_id_.sid;
        sb->seq_num = seq_num;
        sb->peer_ack_timerout = peer_ack_timerout_;
        sb->first_send_time = timer_get_current_time();

        ret = manager_->get_socket()->send(sb);
        if (sb->buf.length == ret) {
            send_buffer_list_[sb->seq_num] = sb;
        } else {
            buffer_destroy(sb->buf);
            alloc_delete_object(sb);
        }

    } while (0);
    return ret;
}
i32 Session::disconnect(ui16 reason)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (state_ <= RDPSESSIONSTATUS_INIT ) {
            ret = RDPERROR_SESSION_BADSTATE;
            break;
        }
        state_ = RDPSESSIONSTATUS_INIT;

        protocol_disconnect pack;
        pack.reason = reason;
        protocol_set_header(&pack);

        send_buffer sb ;
        memset(&sb, 0, sizeof(send_buffer));
        sb.buf.ptr = (ui8*)&pack;
        sb.buf.capcity = sizeof(protocol_disconnect);
        sb.buf.length = sb.buf.capcity;
        sb.addr = get_addr();
        sb.session_id = session_id_.sid;
        sb.seq_num = 0;

        ret = manager_->get_socket()->send(&sb);
    } while (0);

    for (send_buffer_list::iterator it = send_buffer_list_.begin();
            it != send_buffer_list_.end(); ++it) {
        send_buffer* sb = it->second;
        buffer_destroy(sb->buf);
        alloc_delete_object(sb);
    }
    send_buffer_list_.clear();

    return ret;
}
i32 Session::send(const ui8* buf, ui16 buf_len, ui32 flags)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (state_ != RDPSESSIONSTATUS_CONNECTED ) {
            ret = RDPERROR_SESSION_BADSTATE;
            break;
        }
        if (flags & RDPSESSIONSENDFLAG_ACK) {
            if (seq_num_ >= 0xffffffff) {
                seq_num_ = 1;
            }
            ui32 seq_num = seq_num_++;

            protocol_data pack;
            pack.seq_num = seq_num;
            pack.seq_num_ack = 0;
            pack.flags = 0;
            pack.data_size = buf_len;
            protocol_set_header(&pack);

            send_buffer* sb = alloc_new_object<send_buffer>();
            memset(sb, 0, sizeof(send_buffer));
            sb->buf = buffer_create(sizeof(protocol_data)+buf_len);
            memcpy(sb->buf.ptr, &pack, sizeof(protocol_data));
            memcpy(sb->buf.ptr + sizeof(protocol_data), buf, buf_len);
            sb->addr = get_addr();
            sb->session_id = session_id_.sid;
            sb->seq_num = seq_num;
            sb->peer_ack_timerout = peer_ack_timerout_;
            sb->first_send_time = timer_get_current_time();

            ret = manager_->get_socket()->send(sb);
            if (sb->buf.length == ret) {
                send_buffer_list_[sb->seq_num] = sb;
            } else {
                buffer_destroy(sb->buf);
                alloc_delete_object(sb);
            }
        } else {
            protocol_data_noack pack;
            pack.data_size = buf_len;
            protocol_set_header(&pack);

            send_buffer sb;
            memset(&sb, 0, sizeof(send_buffer));
            sb.buf = buffer_create(sizeof(protocol_data_noack)+buf_len);
            memcpy(sb.buf.ptr, &pack, sizeof(protocol_data_noack));
            memcpy(sb.buf.ptr + sizeof(protocol_data_noack), buf, buf_len);
            sb.addr = get_addr();
            sb.session_id = session_id_.sid;
            sb.seq_num = 0;

            ret = manager_->get_socket()->send(&sb);

            buffer_destroy(sb.buf);
        }
    } while (0);
    return ret;
}
i32 Session::heartbeat()
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        protocol_heartbeat pack;
        protocol_set_header(&pack);

        send_buffer sb;
        memset(&sb, 0, sizeof(send_buffer));
        sb.buf.ptr = (ui8*)&pack;
        sb.buf.capcity = sizeof(protocol_heartbeat);
        sb.buf.length = sb.buf.capcity;
        sb.addr = get_addr();
        sb.session_id = session_id_.sid;
        sb.seq_num = 0;

        ret = manager_->get_socket()->send(&sb);
    } while (0);
    return ret;
}
i32 Session::ack(ui32* seq_num_ack, ui8 seq_num_ack_count)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (state_ != RDPSESSIONSTATUS_CONNECTED) {
            ret = RDPERROR_SESSION_BADSTATE;
            break;
        }
        if (0 == seq_num_ack_count) {
            break;
        }
        ui8 seq_nums[sizeof(protocol_ack)+255 * sizeof(ui32)];

        protocol_ack pack;
        pack.win_size = 0;
        pack.seq_num_ack_count = seq_num_ack_count;
        protocol_set_header(&pack);

        send_buffer sb;
        memset(&sb, 0, sizeof(send_buffer));
        sb.buf.ptr = (ui8*)seq_nums;
        sb.buf.length = sizeof(protocol_ack)+seq_num_ack_count*sizeof(ui32);
        sb.buf.capcity = sb.buf.length;
        memcpy(sb.buf.ptr, &pack, sizeof(protocol_ack));
        memcpy(sb.buf.ptr + sizeof(protocol_ack), (char*)seq_num_ack, seq_num_ack_count*sizeof(ui32));
        sb.addr = get_addr();
        sb.session_id = session_id_.sid;
        sb.seq_num = 0;

        ret = manager_->get_socket()->send(&sb);
    } while (0);
    return ret;
}
i32 Session::ctrl_ack(ui32 seq_num_ack, ui16 cmd, ui16 error)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        protocol_ctrl_ack pack;
        pack.seq_num_ack = seq_num_ack;
        pack.cmd = cmd;
        pack.error = error;
        protocol_set_header(&pack);

        send_buffer sb;
        memset(&sb, 0, sizeof(send_buffer));
        sb.buf.ptr = (ui8*)&pack;
        sb.buf.length = sizeof(protocol_ctrl_ack);
        sb.buf.capcity = sb.buf.length;
        sb.addr = get_addr();
        sb.session_id = session_id_.sid;
        sb.seq_num = 0;

        ret = manager_->get_socket()->send(&sb);
    } while (0);
    return ret;
}
i32 Session::connect_ack(ui32 seq_num_ack, ui16 error)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        const rdp_socket_create_param& param = manager_->get_socket()->get_create_param();

        protocol_connect_ack pack;
        pack.win_size = 0;
        pack.seq_num_ack = seq_num_ack;
        pack.error = error;
        pack.ack_timeout = param.ack_timeout;
        pack.heart_beat_timeout = param.heart_beat_timeout;
        protocol_set_header(&pack);

        send_buffer sb;
        memset(&sb, 0, sizeof(send_buffer));
        sb.buf.ptr = (ui8*)&pack;
        sb.buf.length = sizeof(protocol_connect_ack);
        sb.buf.capcity = sb.buf.length;
        sb.addr = get_addr();
        sb.session_id = session_id_.sid;
        sb.seq_num = 0;

        ret = manager_->get_socket()->send(&sb);
    } while (0);
    return ret;
}
void Session::on_recv(protocol_header* ph)
{
    if (ph->protocol == proto_ack) {
        on_ack((protocol_ack*)ph);
    } else if (ph->protocol == proto_ctrl) {
        on_ctrl((protocol_ctrl*)ph);
    } else if (ph->protocol == proto_ctrl_ack) {
        on_ctrl_ack((protocol_ctrl_ack*)ph);
    } else if (ph->protocol == proto_connect) {
        on_connect((protocol_connect*)ph);
    } else if (ph->protocol == proto_connect_ack) {
        on_connect_ack((protocol_connect_ack*)ph);
    } else if (ph->protocol == proto_disconnect) {
        on_disconnect( (protocol_disconnect*)ph);
    } else if (ph->protocol == proto_heartbeat) {
        on_heartbeat((protocol_heartbeat*)ph);
    } else if (ph->protocol == proto_data) {
        on_data( (protocol_data*)ph);
    } else if (ph->protocol == proto_data_noack) {
        on_data_noack((protocol_data_noack*)ph);
    }

    recv_last_ = timer_get_current_time();
}
void Session::on_ack(protocol_ack* p)
{
    peer_window_size_ = p->win_size;

    ui32* seq_num_ack = (ui32*)(p + 1);
    on_handle_ack(seq_num_ack, p->seq_num_ack_count);
}
void Session::on_ctrl(protocol_ctrl* p)
{
    on_handle_ctrl( p);
}
void Session::on_ctrl_ack(protocol_ctrl_ack* p)
{
    if (p->seq_num_ack != 0) {
        ui32 seq_num_ack[1] = { p->seq_num_ack };
        on_handle_ack(seq_num_ack, _countof(seq_num_ack));
    }
    on_handle_ctrl_ack( p);
}
void Session::on_connect(protocol_connect* p)
{
    on_handle_connect( p);
}
void Session::on_connect_ack( protocol_connect_ack* p)
{
    peer_window_size_ = p->win_size;

    if (p->seq_num_ack != 0) {
        ui32 seq_num_ack[1] = { p->seq_num_ack };
        on_handle_ack(seq_num_ack, _countof(seq_num_ack));
    }
    on_handle_connect_ack( p);
}
void Session::on_disconnect(protocol_disconnect* p)
{
    on_handle_disconnect( p);
}
void Session::on_heartbeat(protocol_heartbeat* p)
{
    on_handle_heartbeat( p);
}
void Session::on_data(protocol_data* p)
{
    if (p->seq_num_ack != 0) {
        ui32 seq_num_ack[1] = { p->seq_num_ack };
        on_handle_ack(seq_num_ack, _countof(seq_num_ack) );
    }
    on_handle_data( p);
}
void Session::on_data_noack( protocol_data_noack* p)
{
    on_handle_data_noack( p);
}
void Session::on_handle_ack(ui32* seq_num_ack, ui8 seq_num_ack_count)
{
    const rdp_socket_create_param& sparam = manager_->get_socket()->get_create_param();

    for (ui8 i = 0; i < seq_num_ack_count; ++i) {
        ui32 seq_num = seq_num_ack[i];
        send_buffer_list::iterator it = send_buffer_list_.find(seq_num);
        if (it == send_buffer_list_.end()) {
            continue;
        }

        //暂时不考虑数据包分片(不分片,不组片),如果收到确认,直接从发送队列中删除发送的消息
        send_buffer* sb = it->second;
        send_buffer_list_.erase(it);

        buffer_destroy(sb->buf);
        alloc_delete_object(sb);
    }

    if (sparam.on_send) {
        rdp_on_send_param param;
        param.userdata = sparam.userdata;
        param.err = 0;
        param.sock = manager_->get_socket()->get_rdpsocket();
        param.session_id = session_id_.sid;
        param.local_send_queue_size = send_buffer_list_.size();
        param.peer_window_size = peer_window_size_;

        sparam.on_send(&param);
    }
}
void Session::on_handle_ctrl(protocol_ctrl* p)
{

}
void Session::on_handle_ctrl_ack(protocol_ctrl_ack* p)
{

}
void Session::on_handle_connect(protocol_connect* p)
{
    if (session_is_in_come(session_id_.sid)) {
        state_ = RDPSESSIONSTATUS_CONNECTED;
    }
    connect_ack(p->seq_num, protocol_connect_ack::connect_ack_success);
}
void Session::on_handle_connect_ack(protocol_connect_ack* p)
{
    const rdp_socket_create_param& sparam = manager_->get_socket()->get_create_param();

    state_ = RDPSESSIONSTATUS_CONNECTED;

    rdp_on_connect_param param;
    param.userdata = sparam.userdata;
    param.sock = manager_->get_socket()->get_rdpsocket();
    param.err = 0;
    param.session_id = session_id_.sid;

    sparam.on_connect(&param);
}
void Session::on_handle_disconnect(protocol_disconnect* p)
{
    const rdp_socket_create_param& sparam = manager_->get_socket()->get_create_param();

    state_ = RDPSESSIONSTATUS_INIT;

    rdp_on_disconnect_param param;
    param.userdata = sparam.userdata;
    param.err = RDPERROR_SUCCESS;
    param.reason = p->reason;
    param.sock = manager_->get_socket()->get_rdpsocket();
    param.session_id = session_id_.sid;

    sparam.on_disconnect(&param);
}
void Session::on_handle_heartbeat(protocol_heartbeat* p)
{
    if (session_is_in_come(session_id_.sid)) {
        heartbeat();
    }
}
void Session::on_handle_data(protocol_data* p)
{
    const rdp_socket_create_param& sparam = manager_->get_socket()->get_create_param();

    rdp_on_recv_param param;
    param.userdata = sparam.userdata;
    param.sock = manager_->get_socket()->get_rdpsocket();
    param.session_id = session_id_.sid;
    param.buf = (ui8*)(p + 1);
    param.buf_len = p->data_size;

    ui32 seq_num_ack[1] = { p->seq_num };
    ack(seq_num_ack, _countof(seq_num_ack));

    sparam.on_recv(&param);
}
void Session::on_handle_data_noack(protocol_data_noack* p)
{
    const rdp_socket_create_param& sparam = manager_->get_socket()->get_create_param();

    rdp_on_recv_param param;
    param.userdata = sparam.userdata;
    param.sock = manager_->get_socket()->get_rdpsocket();
    param.session_id = session_id_.sid;
    param.buf = (ui8*)(p + 1);
    param.buf_len = p->data_size;
    //no_ack类的数据包,直接提交给上层服务
    sparam.on_recv(&param);
}
void Session::on_update(const timer_val& now)
{
    const int max_times = 10;
    const rdp_socket_create_param& scparam = manager_->get_socket()->get_create_param();

    do {
        if (timer_is_empty(recv_last_)) {
            recv_last_ = now;
        } else {
            i64 sec = timer_sub_sec(now, recv_last_);
            if (sec >= scparam.heart_beat_timeout*4/5) {
                if (!session_is_in_come(session_id_.sid)) {
                    heartbeat();
                }
            }
            if (sec >= scparam.heart_beat_timeout) {
                state_ = RDPSESSIONSTATUS_INIT;

                rdp_on_disconnect_param dparam;
                dparam.userdata = scparam.userdata;
                dparam.err = RDPERROR_SESSION_HEARTBEATTIMEOUT;
                dparam.reason = 0;
                dparam.sock = manager_->get_socket()->get_rdpsocket();
                dparam.session_id = session_id_.sid;
                scparam.on_disconnect(&dparam);
                break;
            }
        }

        if (send_buffer_list_.empty()) {
            break;
        }

        rdp_on_send_param send_param;
        send_param.userdata = scparam.userdata;
        send_param.sock = manager_->get_socket()->get_rdpsocket();
        send_param.peer_window_size = 0;

        for (send_buffer_list::iterator it = send_buffer_list_.begin();
                it != send_buffer_list_.end();) {
            send_buffer* sb = it->second;

            ui64 interval = timer_sub_msec(now, sb->send_time);
            if (interval < sb->peer_ack_timerout && interval < scparam.ack_timeout) {
                ++it;
                continue;
            }
            i32 ret = manager_->get_socket()->send(sb);

            protocol_header* ph = (protocol_header*)sb->buf.ptr;
            if (ph->protocol == proto_connect) {
                protocol_connect* p = (protocol_connect*)ph;
                ui64 interval_from_begin = timer_sub_msec(now, sb->first_send_time);

                if (interval_from_begin >= p->connect_timeout ||
                        ret != sb->buf.length ) {

                    state_ = RDPSESSIONSTATUS_INIT;

                    rdp_on_connect_param cparam;
                    cparam.userdata = scparam.userdata;
                    cparam.err = ret != sb->buf.length ? ret : RDPERROR_SESSION_CONNTIMEOUT;
                    cparam.sock = send_param.sock;
                    cparam.session_id = sb->session_id;
                    scparam.on_connect(&cparam);

                    break;
                }
            }

            if (ret != sb->buf.length ) {
                it = send_buffer_list_.erase(it);

                if (scparam.on_send) {
                    send_param.err = ret;
                    send_param.session_id = sb->session_id;
                    send_param.local_send_queue_size = send_buffer_list_.size();
                    scparam.on_send(&send_param);
                }

                buffer_destroy(sb->buf);
                alloc_delete_object(sb);
            } else {
                ++it;
            }
        }

    } while (0);


    if (state_ == RDPSESSIONSTATUS_INIT) {
        for (send_buffer_list::iterator it = send_buffer_list_.begin();
                it != send_buffer_list_.end(); ++it) {
            send_buffer* sb = it->second;

            buffer_destroy(sb->buf);
            alloc_delete_object(sb);
        }
        send_buffer_list_.clear();
    }
}