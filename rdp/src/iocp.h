#ifndef IOCP_H
#define IOCP_H

#ifdef PLATFORM_OS_WINDOWS

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"

#include "socket_api.h"

i32 iocp_create(recv_result_callback cb, recv_result_timeout_callback tcb);
i32 iocp_destroy();
i32 iocp_attach(ui8 slot, SOCKET sock, bool v4);
i32 iocp_detach(SOCKET sock);

i32 iocp_recv(ui32 timeout);
#endif

#endif
