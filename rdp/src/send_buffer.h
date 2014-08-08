#ifndef SENDBUFFER_H
#define SENDBUFFER_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "buffer.h"
#include "socket.h"

typedef struct send_buffer {  
    buffer    buf;
    i32       flags;
    sockaddr* addr;

    socket_handle sh;
    SOCKET        sock;
    RDPSESSIONID  session_id;
    ui32          seq_num;
    timer_val     send_time;
}send_buffer;
 
typedef struct send_buffer_ex : public send_buffer{
    ui16 send_times;
}send_buffer_ex;

#endif