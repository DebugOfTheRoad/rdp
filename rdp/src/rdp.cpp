#include "../include/rdp.h"
#include "core.h"

i32 rdp_startup(rdp_startup_param* param)
{
    return core_startup(param);
}
i32 rdp_startup_get_param(rdp_startup_param* param)
{
    return core_startup_get_param(param);
}
i32 rdp_cleanup()
{
    return core_cleanup();
}
i32 rdp_getsyserror()
{
    return core_getsyserror();
}
i32 rdp_getsyserrordesc(i32 err, char* desc, ui32* desc_len)
{
    return core_getsyserrordesc(err, desc, desc_len);
}
i32 rdp_socket_create(rdp_socket_create_param* param, RDPSOCKET* sock)
{
    return core_socket_create(param, sock);
}

i32 rdp_socket_get_create_param(RDPSOCKET sock, rdp_socket_create_param* param)
{
    return core_socket_get_create_param(sock, param);
}
i32 rdp_socket_get_state(RDPSOCKET sock, ui32* state)
{
    return core_socket_get_state(sock, state);
}
i32 rdp_socket_close(RDPSOCKET sock)
{
    return core_socket_close(sock);
}
i32 rdp_socket_bind(RDPSOCKET sock, const char* ip, ui32 port)
{
    return core_socket_bind(sock, ip, port);
}
i32 rdp_socket_listen(RDPSOCKET sock)
{
    return core_socket_listen(sock);
}
i32 rdp_socket_connect(RDPSOCKET sock, const char* ip, ui32 port, ui32 timeout, const ui8* buf, ui16 buf_len, RDPSESSIONID* session_id)
{
    return core_socket_connect(sock, ip, port, timeout, buf, buf_len, session_id);
}

i32 rdp_session_close(RDPSOCKET sock, RDPSESSIONID session_id, i32 reason)
{
    return core_session_close(sock, session_id, reason);
}
i32 rdp_session_get_state(RDPSOCKET sock, RDPSESSIONID session_id, ui32* state)
{
    return core_session_get_state(sock, session_id, state);
}
i32 rdp_session_send(RDPSOCKET sock, RDPSESSIONID session_id, const ui8* buf, ui16 buf_len, ui32 flags)
{
    return core_session_send(sock, session_id, buf, buf_len, flags);
}
bool rdp_session_is_in_come(RDPSESSIONID session_id)
{
    return core_session_is_in_come(session_id);
}
i32 rdp_udp_send(RDPSOCKET sock, const char* ip, ui32 port, const ui8* buf, ui16 buf_len)
{
    return core_udp_send(sock, ip, port, buf, buf_len);
}
i32 rdp_addr_to(const sockaddr* addr, ui32 addrlen, char* ip, ui32* iplen, ui32* port)
{
    return core_addr_to(addr, addrlen, ip, iplen, port);
}