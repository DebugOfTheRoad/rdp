#ifndef SENDBUFFER_H
#define SENDBUFFER_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "buffer.h"
#include "timer.h"
#include "socket_api.h"


typedef struct send_buffer {  
    SOCKET        socket;
    buffer        buf;
    sockaddr*     addr;

    RDPSESSIONID  session_id;
    ui32          seq_num;
    ui16          peer_ack_timerout;

    timer_val     first_send_time;
    timer_val     send_time;
}send_buffer;
 
 

#endif