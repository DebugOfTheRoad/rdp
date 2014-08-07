#ifndef RDP_H
#define RDP_H

#include "lint.h"
#include "rdp_def.h"
#include "platform.h"

#define RDP_API

RDP_API i32 rdp_startup(rdp_startup_param* param);
RDP_API i32 rdp_startup_get_param(rdp_startup_param* param);
RDP_API i32 rdp_cleanup();
RDP_API i32 rdp_getsyserror();
RDP_API i32 rdp_getsyserrordesc(i32 err, char* desc, ui32 desc_len);

RDP_API i32 rdp_socket_create(rdp_socket_create_param* param, RDPSOCKET* sock);
RDP_API i32 rdp_socket_get_create_param(RDPSOCKET sock, rdp_socket_create_param* param);
RDP_API i32 rdp_socket_get_state(RDPSOCKET sock, ui32* state);
RDP_API i32 rdp_socket_close(RDPSOCKET sock);
RDP_API i32 rdp_socket_bind(RDPSOCKET sock, const char* ip = 0, ui32 port = 0);
RDP_API i32 rdp_socket_listen(RDPSOCKET sock);
RDP_API i32 rdp_socket_connect(RDPSOCKET sock, const char* ip, ui32 port, ui32 timeout,
                               RDPSESSIONID* session_id, const ui8* buf = 0, ui32 buf_len = 0);

RDP_API i32 rdp_session_close(RDPSOCKET sock, RDPSESSIONID session_id);
RDP_API i32 rdp_session_get_state(RDPSOCKET sock, RDPSESSIONID session_id, ui32* state);
RDP_API i32 rdp_session_send(RDPSOCKET sock, RDPSESSIONID session_id, const ui8* buf, ui32 buf_len,
                             bool need_ack = true, bool in_order = true,
                             ui32* local_send_queue_size = 0, ui32* peer_unused_recv_queue_size = 0);
RDP_API bool rdp_session_is_income(RDPSESSIONID session_id);

RDP_API i32 rdp_udp_send(RDPSOCKET sock, const char* ip, ui32 port, const ui8* buf, ui32 buf_len);

RDP_API i32 rdp_addr_to(const sockaddr* addr, ui32 addrlen, char* ip, ui32 iplen, ui32* port);

#endif