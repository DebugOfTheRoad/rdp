#include "core.h"
#include "thread.h"
#include "socket.h"

static bool s_init = false;
i32 core_startup(rdp_startup_param* param)
{
    i32 ret = RDPERROR_SUCCESS;
    do 
    {
        ret = thread_startup();
        if (ret != RDPERROR_SUCCESS) {
            break;
        }
        ret = socket_startup(param);
        if (ret != RDPERROR_SUCCESS) {
            break;
        }
        s_init = true;
    } while (0);
   
    return ret;
}
i32 core_startup_get_param(rdp_startup_param* param)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_startup_get_param(param);
}
i32 core_cleanup()
{
    socket_cleanup();
    thread_cleanup();
    return RDPERROR_SUCCESS;
}
i32 core_getsyserror()
{
    return socket_getsyserror();
}
i32 core_getsyserrordesc(i32 err, char* desc, ui32 desc_len)
{
    return socket_getsyserrordesc(err, desc, desc_len);
}
i32 core_socket_create(rdp_socket_create_param* param, RDPSOCKET* sock)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    if (!sock) {
        return RDPERROR_INVALIDPARAM;
    }
    socket_handle handle = 0;
    i32 ret = socket_create(param, &handle);
    if (ret >= 0) {
        *sock = socket_handle2RDPSOCKET(handle);
    }
    return ret;
}

i32  core_socket_get_create_param(RDPSOCKET sock, rdp_socket_create_param* param)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_get_create_param(RDPSOCKET2socket_handle(sock), param);
}
i32 core_socket_get_state(RDPSOCKET sock, ui32* state)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_get_state(RDPSOCKET2socket_handle(sock), state);
}
i32 core_socket_close(RDPSOCKET sock)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_destroy(RDPSOCKET2socket_handle(sock));
}
i32 core_socket_bind(RDPSOCKET sock, const char* ip, ui32 port)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_bind(RDPSOCKET2socket_handle(sock), ip, port);
}
i32 core_socket_listen(RDPSOCKET sock)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_listen(RDPSOCKET2socket_handle(sock));
}
i32 core_socket_connect(RDPSOCKET sock, const char* ip, ui32 port, ui32 timeout,
    RDPSESSIONID* session_id, const ui8* buf, ui16 buf_len)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_connect(RDPSOCKET2socket_handle(sock), ip, port, timeout, session_id, buf, buf_len);
}

i32 core_session_close(RDPSOCKET sock, RDPSESSIONID session_id)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_session_close(RDPSOCKET2socket_handle(sock), session_id);
}
i32 core_session_get_state(RDPSOCKET sock, RDPSESSIONID session_id, ui32* state)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_session_get_state(RDPSOCKET2socket_handle(sock), session_id, state);
}
i32 core_session_send(RDPSOCKET sock, RDPSESSIONID session_id, const ui8* buf, ui16 buf_len,
    ui32 flags,
    ui32* local_send_queue_size, ui32* peer_unused_recv_queue_size)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_session_send(RDPSOCKET2socket_handle(sock), session_id, buf, buf_len, 
        flags,
        local_send_queue_size, peer_unused_recv_queue_size);
}
bool core_session_is_income(RDPSESSIONID session_id)
{
    return socket_session_is_income(session_id);
}
i32 core_udp_send(RDPSOCKET sock, const char* ip, ui32 port, const ui8* buf, ui16 buf_len)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_udp_send(RDPSOCKET2socket_handle(sock), ip, port, buf, buf_len);
}
i32 core_addr_to(const sockaddr* addr, ui32 addrlen, char* ip, ui32 iplen, ui32* port)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    return socket_addr_to(addr, addrlen, ip, iplen, port);
}