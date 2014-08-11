#ifndef RECV_H
#define RECV_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "socket_api.h"

 
typedef struct recv_result {
    ui8       slot;
    SOCKET    sock;
    sockaddr* addr;
    buffer    buf;
    timer_val now;
} recv_result;
 
typedef void(*recv_result_timeout_callback)(thread_handle, const timer_val&);
typedef void(*recv_result_callback) (thread_handle handle, recv_result*);

#endif