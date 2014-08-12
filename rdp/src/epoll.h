#ifndef EPOLL_H
#define EPOLL_H

#ifdef PLATFORM_OS_LINUX

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"

#include "socket_api.h"

i32 epoll_create(recv_result_callback cb, recv_result_timeout_callback tcb);
i32 epoll_destroy();
i32 epoll_attach(ui8 slot, SOCKET sock, bool v4);
i32 epoll_detach(SOCKET sock);

i32 epoll_recv(ui32 timeout);
#endif

#endif
