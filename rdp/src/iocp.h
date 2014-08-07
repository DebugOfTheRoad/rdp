#ifndef IOCP_H
#define IOCP_H

#ifdef PLATFORM_OS_WINDOWS

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"

#include "socket_api.h"

i32 iocp_create(recv_result_callback cb);
i32 iocp_destroy();
i32 iocp_attach(SOCKET sock, bool v4);
i32 iocp_detach(SOCKET sock);

#endif

#endif
