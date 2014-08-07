#ifndef SESSION_H
#define SESSION_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"

#include "socket_api.h"
#include "socket.h"
#include "send_buffer.h"
#include "recv_buffer.h"
#include "recv.h"
#include "thread.h"

#include <map>

typedef std::map<ui32, send_buffer_ex*> send_buffer_list;
typedef std::map<ui32, recv_buffer_ex*> recv_buffer_list;

typedef struct session {
    socket_handle     sh;
    RDPSESSIONID      session_id; //
    i8                state;      //RDPSTATUS
    sockaddr*         addr;       //remote addr:sockaddr_in sockaddr_in6
    send_buffer_list* send_buf_list;
    recv_buffer_list* recv_buf_list;
    ui32              send_seq_num_now;
    timer_val         heart_beat;
    ui32              peer_window_size;
} session;


session* session_create(socket_handle sh, RDPSESSIONID session_id, const sockaddr* addr);
void session_destroy(session* sess);
i32 session_send_ctrl(session* sess, ui16 cmd);
void session_send_ctrl_ack(session* sess, ui16 cmd, ui16 error);
void session_send_ack(session* sess, ui32* seq_num_ack, ui8 seq_num_ack_count);
i32 session_send_connect(session* sess, const ui8* data=0, ui32 data_size=0);
void session_send_connect_ack(session* sess, ui16 error, ui16 heart_beat_timeout);
i32 session_send_disconnect(session* sess, ui16 reason);
i32 session_send_heartbeat(session* sess);
i32 session_send_data(session* sess, const ui8* data, ui16 data_size,
    bool need_ack, bool in_order,
    ui32* local_send_queue_size, ui32* peer_unused_recv_queue_size);
void session_handle_recv(session* sess, recv_result* result);

#endif