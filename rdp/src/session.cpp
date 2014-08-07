#include "session.h"
#include "thread.h"
#include "send_buffer.h"
#include "recv_buffer.h"
#include "send.h"
#include "protocol.h"

#include <map>


session* session_create(socket_handle sh, RDPSESSIONID session_id, const sockaddr* addr)
{
    socket_info* so = socket_get_socket_info(sh);
    session* sess = new session;
    memset(sess, 0, sizeof(session));
    sess->sh = sh;
    sess->session_id = session_id;
    sess->state = RDPSESSIONSTATUS_INIT ;
    sess->send_buf_list = new send_buffer_list;
    sess->recv_buf_list = new recv_buffer_list;
    sess->addr = socket_api_addr_create(addr);
    sess->heart_beat = timer_get_current_time();
    sess->peer_window_size = 0;

    return sess;
}
void session_destroy(session* sess)
{
    if (!sess) {
        return;
    }
    socket_info* so = socket_get_socket_info(sess->sh);

    send_buffer_list* send_buf_list = sess->send_buf_list;
    recv_buffer_list* recv_buf_list = sess->recv_buf_list;
    for (send_buffer_list::iterator it = send_buf_list->begin();
            it != send_buf_list->end(); ++it) {
        send_buffer_ex* buf = it->second;

        buffer_destroy(buf->buf);
        socket_api_addr_destroy(buf->addr);
        delete buf;
    }
    send_buf_list->clear();

    for (recv_buffer_list::iterator it = recv_buf_list->begin();
            it != recv_buf_list->end(); ++it) {
        recv_buffer_ex* buf = it->second;
        //buffer_destroy(buf->b);
    }
    recv_buf_list->clear();

    delete sess->send_buf_list;
    delete sess->recv_buf_list;
    socket_api_addr_destroy(sess->addr);
    delete sess;
}
void session_send_ack(session* sess, ui32* seq_num_ack, ui8 seq_num_ack_count)
{
    if (0 == seq_num_ack_count) {
        return;
    }
    ui8 seq_nums[sizeof(protocol_ack)+255 * sizeof(ui32)];

    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_ack pack;
    pack.win_size = so->create_param.max_recv_queue_size - sess->recv_buf_list->size();
    pack.seq_num = seq_num;
    pack.seq_num_ack_count = seq_num_ack_count;

    send_buffer_ex sb ;
    memset(&sb, 0, sizeof(send_buffer_ex));
    sb.buf.ptr = (ui8*)seq_nums;
    sb.buf.length = sizeof(protocol_ack)+seq_num_ack_count*sizeof(ui32);
    sb.buf.capcity = sb.buf.length;
    memcpy(sb.buf.ptr, &pack, sizeof(protocol_ack));
    memcpy(sb.buf.ptr + sizeof(protocol_ack), (char*)seq_num_ack, seq_num_ack_count*sizeof(ui32));
    sb.addr = sess->addr;
    sb.sh = sess->sh;
    sb.sock = so->sock;
    sb.session_id = sess->session_id;
    sb.seq_num = seq_num;
    sb.send_times = 1;

    // printf("send ack %d\n", (i32)seq_num_ack[0]);
    send_send(&sb);
}
i32 session_send_ctrl(session* sess, ui16 cmd)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_ctrl pack;
    pack.seq_num = seq_num;
    pack.cmd = cmd;

    send_buffer_ex* sb = new send_buffer_ex;
    memset(sb, 0, sizeof(send_buffer_ex));
    sb->buf = buffer_create((const ui8*)&pack, sizeof(protocol_ctrl));
    sb->addr = sess->addr;
    sb->sh = sess->sh;
    sb->sock = so->sock;
    sb->session_id = sess->session_id;
    sb->seq_num = seq_num;
    sb->send_times = 1;

    i32 ret = send_send(sb);
    if (sb->buf.length == ret) {
        (*sess->send_buf_list)[seq_num] = sb;
    } else {
        buffer_destroy(sb->buf);
        delete sb;
    }
    return ret;
}
void session_send_ctrl_ack(session* sess, ui16 cmd, ui16 error)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_ctrl_ack pack;
    pack.win_size = so->create_param.max_recv_queue_size - sess->recv_buf_list->size();
    pack.seq_num_ack = 0;
    pack.seq_num = seq_num;
    pack.cmd = cmd;
    pack.error = error;

    send_buffer_ex sb;
    memset(&sb, 0, sizeof(send_buffer_ex));
    sb.buf.ptr = (ui8*)&pack;
    sb.buf.length = sizeof(protocol_ctrl_ack);
    sb.buf.capcity = sb.buf.length;
    sb.addr = sess->addr;
    sb.sh = sess->sh;
    sb.sock = so->sock;
    sb.session_id = sess->session_id;
    sb.seq_num = seq_num;
    sb.send_times = 1;

    send_send(&sb);
}
i32 session_send_connect(session* sess, const ui8* data, ui32 data_size)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_connect pack;
    pack.seq_num = seq_num;

    send_buffer_ex* sb = new send_buffer_ex;
    memset(sb, 0, sizeof(send_buffer_ex));
    sb->buf = buffer_create(sizeof(protocol_connect)+data_size);
    memcpy(sb->buf.ptr, &pack, sizeof(protocol_connect));
    memcpy(sb->buf.ptr + sizeof(protocol_connect), data, data_size);
    sb->addr = sess->addr;
    sb->sh = sess->sh;
    sb->sock = so->sock;
    sb->session_id = sess->session_id;
    sb->seq_num = seq_num;
    sb->send_times = 1;

    i32 ret = send_send(sb);
    if (sb->buf.length == ret) {
        (*sess->send_buf_list)[seq_num] = sb;
    } else {
        buffer_destroy(sb->buf);
        delete sb;
    }
    return ret;
}
void session_send_connect_ack(session* sess, ui32 seq_num_ack, ui16 error, ui16 heart_beat_timeout)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_connect_ack pack;
    pack.win_size = so->create_param.max_recv_queue_size - sess->recv_buf_list->size();
    pack.seq_num_ack = 0;
    pack.seq_num = seq_num;
    pack.seq_num_ack = seq_num_ack;
    pack.error = error;
    pack.heart_beat_timeout = heart_beat_timeout;

    send_buffer_ex sb;
    memset(&sb, 0, sizeof(send_buffer_ex));
    sb.buf.ptr = (ui8*)&pack;
    sb.buf.length = sizeof(protocol_connect_ack);
    sb.buf.capcity = sb.buf.length;
    sb.addr = sess->addr;
    sb.sh = sess->sh;
    sb.sock = so->sock;
    sb.session_id = sess->session_id;
    sb.seq_num = seq_num;
    sb.send_times = 1;

    send_send(&sb);
}
i32 session_send_disconnect(session* sess, ui16 reason)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_disconnect pack;
    pack.seq_num = seq_num;
    pack.reason = reason;

    send_buffer_ex* sb = new send_buffer_ex;
    memset(sb, 0, sizeof(send_buffer_ex));
    sb->buf = buffer_create((const ui8*)&pack, sizeof(protocol_disconnect));
    sb->addr = sess->addr;
    sb->sh = sess->sh;
    sb->sock = so->sock;
    sb->session_id = sess->session_id;
    sb->seq_num = seq_num;
    sb->send_times = 1;

    i32 ret = send_send(sb);
    if (sb->buf.length == ret) {
        (*sess->send_buf_list)[seq_num] = sb;
    } else {
        buffer_destroy(sb->buf);
        delete sb;
    }
    return ret;
}
i32 session_send_heartbeat(session* sess)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_heartbeat pack;
    pack.seq_num = seq_num;

    send_buffer_ex* sb = new send_buffer_ex;
    memset(sb, 0, sizeof(send_buffer_ex));
    sb->buf = buffer_create((const ui8*)&pack, sizeof(protocol_heartbeat));
    sb->addr = sess->addr;
    sb->sh = sess->sh;
    sb->sock = so->sock;
    sb->session_id = sess->session_id;
    sb->seq_num = seq_num;
    sb->send_times = 1;

    i32 ret = send_send(sb);
    if (sb->buf.length == ret) {
        (*sess->send_buf_list)[seq_num] = sb;
    } else {
        buffer_destroy(sb->buf);
        delete sb;
    }
    return ret;
}
i32 session_send_data(session* sess, const ui8* data, ui16 data_size,
    bool need_ack, bool in_order,
    ui32* local_send_queue_size, ui32* peer_unused_recv_queue_size)
{
    socket_info* so = socket_get_socket_info(sess->sh);

    if (local_send_queue_size){
        *local_send_queue_size = (ui32)sess->send_buf_list->size();
    }
    if (peer_unused_recv_queue_size){
        *peer_unused_recv_queue_size = sess->peer_window_size;
    }

    if (data_size == 0) {
        return 0;
    }

    ui32 seq_num = ++sess->send_seq_num_now;
    protocol_data pack;
    pack.seq_num_ack = 0;
    pack.seq_num = seq_num;
    pack.ack_timeout = 0;
    pack.frag_num = 0;
    pack.frag_offset = 0;
    pack.data_size = data_size;

    send_buffer_ex* sb = new send_buffer_ex;
    memset(sb, 0, sizeof(send_buffer_ex));
    sb->buf = buffer_create(sizeof(protocol_data)+data_size);
    memcpy(sb->buf.ptr, &pack, sizeof(protocol_data));
    memcpy(sb->buf.ptr + sizeof(protocol_data), data, data_size);
    sb->addr = sess->addr;
    sb->sh = sess->sh;
    sb->sock = so->sock;
    sb->session_id = sess->session_id;
    sb->seq_num = seq_num;
    sb->send_times = 1;

    i32 ret = send_send(sb);
    if (sb->buf.length == ret) {
        (*sess->send_buf_list)[seq_num] = sb;
    } else {
        buffer_destroy(sb->buf);
        delete sb;
    }
    if (local_send_queue_size){
        *local_send_queue_size = (ui32)sess->send_buf_list->size();
    }
    return ret;
}

//////////////////////////////////////////////////////////////////////////
void on_handle_ack(session* sess, ui32* seq_num_ack, ui8 seq_num_ack_count)
{
    for (ui8 i = 0; i < seq_num_ack_count; ++i) {
        ui32 seq_num = seq_num_ack[i];
        //printf("ack seq_num %d\n", seq_num);
        if (seq_num == 0) {
            // printf("ack seq_num 0 %d\n", seq_num);
            continue;
        }
        send_buffer_list::iterator it = sess->send_buf_list->find(seq_num);
        if (it == sess->send_buf_list->end()) {
            //  printf("ack seq_num bad %d\n", seq_num);
            continue;
        }

        //暂时不考虑数据包分片(不分片,不组片),如果收到确认,直接从发送队列中删除发送的消息
        //以后再添加大数据包分片支持
        send_buffer_ex* sb = it->second;
        sess->send_buf_list->erase(it);

        buffer_destroy(sb->buf);
        delete sb;
    }
   
    if (s_socket_startup_param.on_send){
        rdp_on_send_param param;
        param.err = 0;
        param.sock = socket_handle2RDPSOCKET(sess->sh);
        param.session_id = sess->session_id;
        param.local_send_queue_size = sess->send_buf_list->size();
        param.peer_unused_recv_queue_size = sess->peer_window_size;

        s_socket_startup_param.on_send(param);
    }
}
void on_handle_ctrl(session* sess, protocol_ctrl* p)
{

}
void on_handle_ctrl_ack(session* sess, protocol_ctrl_ack* p)
{

}
void on_handle_connect(session* sess, protocol_connect* p)
{
    bool income = socket_session_is_income(sess->session_id);
    if (!income) {
        return;
    }
    sess->state = RDPSESSIONSTATUS_CONNECTED;
    session_send_connect_ack(sess, p->seq_num, protocol_connect_ack::connect_ack_success, 3 * 60);
}
void on_handle_connect_ack(session* sess, protocol_connect_ack* p)
{
    bool income = socket_session_is_income(sess->session_id);
    if (income) {
        return;
    }
    sess->state = RDPSESSIONSTATUS_CONNECTED;
    socket_info* si = socket_get_socket_info(sess->sh);
    rdp_on_connect_param param;
    param.sock = socket_handle2RDPSOCKET(sess->sh);
    param.err = 0;
    param.session_id = sess->session_id;
    s_socket_startup_param.on_connect(param);
}
void on_handle_disconnect(session* sess, protocol_disconnect* p)
{
    sess->state = RDPSESSIONSTATUS_DISCONNECTING;

    rdp_on_disconnect_param param;
    param.err = RDPERROR_SUCCESS;
    param.reason = p->reason;
    param.sock = socket_handle2RDPSOCKET(sess->sh);
    param.session_id = sess->session_id;

    s_socket_startup_param.on_disconnect(param);

    socket_session_close(sess->sh, sess->session_id);
}
void on_handle_heartbeat(session* sess, protocol_heartbeat* p)
{
    sess->heart_beat = timer_get_current_time();
}
void on_handle_data(session* sess, protocol_data* p)
{
    //printf("handle data seq_num %d\n", p->seq_num);
    sess->heart_beat = timer_get_current_time();
 
    rdp_on_recv_param param;
    param.sock = socket_handle2RDPSOCKET(sess->sh);
    param.session_id = sess->session_id;
    param.buf = (ui8*)(p + 1);
    param.buf_len = p->data_size;

    s_socket_startup_param.on_recv(param);

    ui32 seq_num_ack[1] = { p->seq_num };
    session_send_ack(sess, seq_num_ack, _countof(seq_num_ack));
}

void session_on_ack(session* sess, recv_result* result, protocol_ack* p)
{
    if (sizeof(protocol_ack)+p->seq_num_ack_count*sizeof(ui32) != result->buf.length) {
        return;
    }
    sess->peer_window_size = p->win_size;

    ui32* seq_num_ack = (ui32*)(p + 1);
    on_handle_ack(sess, seq_num_ack, p->seq_num_ack_count);

    // char* c = new char[1024];
    // socket_session_send(sess->sh, sess->session_id, (const ui8*)c, 1024);
    // delete[]c;
}
void session_on_ctrl(session* sess, recv_result* result, protocol_ctrl* p)
{
    if (sizeof(protocol_ctrl) != result->buf.length) {
        return;
    }

    on_handle_ctrl(sess, p);
}
void session_on_ctrl_ack(session* sess, recv_result* result, protocol_ctrl_ack* p)
{
    if (sizeof(protocol_ctrl_ack) != result->buf.length) {
        return;
    }
    sess->peer_window_size = p->win_size;

    if (p->seq_num_ack != 0) {
        ui32 seq_num_ack[1] = { p->seq_num_ack };
        on_handle_ack(sess, seq_num_ack, _countof(seq_num_ack));
    }
    on_handle_ctrl_ack(sess, p);
}
void session_on_connect(session* sess, recv_result* result, protocol_connect* p)
{
    if (sizeof(protocol_connect) != result->buf.length) {
        return;
    }
    on_handle_connect(sess, p);
}
void session_on_connect_ack(session* sess, recv_result* result, protocol_connect_ack* p)
{
    if (sizeof(protocol_connect_ack) != result->buf.length) {
        return;
    }
    sess->peer_window_size = p->win_size;

    if (p->seq_num_ack != 0) {
        ui32 seq_num_ack[1] = { p->seq_num_ack };
        on_handle_ack(sess, seq_num_ack, _countof(seq_num_ack));
    }
    on_handle_connect_ack(sess, p);
}
void session_on_disconnect(session* sess, recv_result* result, protocol_disconnect* p)
{
    if (sizeof(protocol_disconnect) != result->buf.length) {
        return;
    }
    on_handle_disconnect(sess, p);
}
void session_on_heartbeat(session* sess, recv_result* result, protocol_heartbeat* p)
{
    if (sizeof(protocol_heartbeat) != result->buf.length) {
        return;
    }
    on_handle_heartbeat(sess, p);
}
void session_on_data(session* sess, recv_result* result, protocol_data* p)
{
    if (sizeof(protocol_data) + p->data_size != result->buf.length) {
        return;
    }
    if (p->seq_num_ack != 0) {
        ui32 seq_num_ack[1] = { p->seq_num_ack };
        on_handle_ack(sess, seq_num_ack, _countof(seq_num_ack));
    }
    on_handle_data(sess, p);
}

void session_handle_recv(session* sess, recv_result* result)
{
    protocol_header* ph = (protocol_header*)result->buf.ptr;
    //sess->heart_beat = timer_get_current_time(); //收到数据即更新心跳
    
    if (ph->protocol_id == proto_ack) {
        session_on_ack(sess, result, (protocol_ack*)ph);
    } else if (ph->protocol_id == proto_ctrl) {
        session_on_ctrl(sess, result, (protocol_ctrl*)ph);
    } else if (ph->protocol_id == proto_ctrl_ack) {
        session_on_ctrl_ack(sess, result, (protocol_ctrl_ack*)ph);
    } else if (ph->protocol_id == proto_connect) {
        session_on_connect(sess, result, (protocol_connect*)ph);
    } else if (ph->protocol_id == proto_connect_ack) {
        session_on_connect_ack(sess, result, (protocol_connect_ack*)ph);
    } else if (ph->protocol_id == proto_disconnect) {
        session_on_disconnect(sess, result, (protocol_disconnect*)ph);
    } else if (ph->protocol_id == proto_heartbeat) {
        session_on_heartbeat(sess, result, (protocol_heartbeat*)ph);
    } else if (ph->protocol_id == proto_data) {
        session_on_data(sess, result, (protocol_data*)ph);
    }
}