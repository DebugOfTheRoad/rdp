#ifndef RECVTHREAD_H
#define RECVTHREAD_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "socket.h"
 
typedef struct recv_result {
    SOCKET    sock;
    sockaddr* addr;
    buffer    buf;
} recv_result;
 
typedef void(*recv_result_timeout)(thread_handle);
typedef void(*recv_result_callback) (thread_handle handle, recv_result*);

#endif