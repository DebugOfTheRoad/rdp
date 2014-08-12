#include "core.h"
#include "thread.h"
#include "socket.h"
#include "session.h"
#include "debug.h"
 
static bool s_init = false;

i32 core_startup(rdp_startup_param* param)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!param) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        debug_startup();
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
    debug_cleanup();
    return RDPERROR_SUCCESS;
}
i32 core_getsyserror()
{
    return socket_api_getsyserror();
}
i32 core_getsyserrordesc(i32 err, char* desc, ui32* desc_len)
{

    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!desc || !desc_len || *desc_len == 1) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }

        desc[0] = 0;

#if !defined(PLATFORM_OS_WINDOWS)
        strerror_r(err, desc, *desc_len);
        desc_len = strlen(desc);
#else
        LPSTR lpMsgBuf = 0;
        ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL, err,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPSTR)&lpMsgBuf, 0, NULL);
        if (lpMsgBuf) {
            char* dst = desc;
            char* src = lpMsgBuf;
            while ((dst - desc < (i32)*desc_len - 1) && *src) {
                *dst++ = *src++;
            }
            *dst = 0;
            *desc_len = (ui32)(dst - desc);
        }

        ::LocalFree(lpMsgBuf);
#endif
    } while (0);

    return ret;
}
i32 core_socket_create(rdp_socket_create_param* param, RDPSOCKET* sock)
{
    if (!s_init) {
        return RDPERROR_NOTINIT;
    }
    if (!param || !sock) {
        return RDPERROR_INVALIDPARAM;
    }
    mutex_handle lock = 0;
    Socket* socket = 0;
    i32 ret = socket_create(param, &socket, lock);
    if (ret >= 0) {
        *sock = socket->get_rdpsocket();
    }
    mutex_unlock(lock);
    return ret;
}

i32  core_socket_get_create_param(RDPSOCKET sock, rdp_socket_create_param* param)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        if (!param) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        *param = socket->get_create_param();
        mutex_unlock(lock);
    } while (0);

    return ret;
}
i32 core_socket_get_state(RDPSOCKET sock, ui32* state)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        if (!state) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        *state = socket->get_state();
        mutex_unlock(lock);
    } while (0);

    return ret;
}
i32 core_socket_close(RDPSOCKET sock)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        socket->destroy();
        mutex_unlock(lock);
    } while (0);

    return  ret;
}
i32 core_socket_bind(RDPSOCKET sock, const char* ip, ui32 port)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->bind(ip, port);
        mutex_unlock(lock);
    } while (0);

    return ret;
}
i32 core_socket_listen(RDPSOCKET sock)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->listen();
        mutex_unlock(lock);
    } while (0);


    return  ret;
}
i32 core_socket_connect(RDPSOCKET sock, const char* ip, ui32 port, ui32 timeout, const ui8* buf, ui16 buf_len, RDPSESSIONID* session_id)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        if (!session_id) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->connect(ip, port, timeout, buf, buf_len, session_id);
        mutex_unlock(lock);
    } while (0);

    return ret;
}
i32 core_socket_recv(ui32 timeout)
{
    return socket_recv(timeout);
}
i32 core_session_close(RDPSOCKET sock, RDPSESSIONID session_id, i32 reason)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->session_close(session_id, reason);
        mutex_unlock(lock);
    } while (0);

    return ret;
}
i32 core_session_get_state(RDPSOCKET sock, RDPSESSIONID session_id, ui32* state)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        if (!state) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->session_get_state(session_id, state);
        mutex_unlock(lock);
    } while (0);

    return ret;
}
i32 core_session_send(RDPSOCKET sock, RDPSESSIONID session_id, const ui8* buf, ui16 buf_len, ui32 flags)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        if (!buf || buf_len == 0) {
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->session_send(session_id, buf, buf_len, flags);
        mutex_unlock(lock);
    } while (0);

    return ret;
}
bool core_session_is_in_come(RDPSESSIONID session_id)
{
    return session_is_in_come(session_id);
}
i32 core_udp_send(RDPSOCKET sock, const char* ip, ui32 port, const ui8* buf, ui16 buf_len)
{
    i32 ret = RDPERROR_SUCCESS;

    do {
        if (!s_init) {
            ret = RDPERROR_NOTINIT;
            break;
        }
        if (!buf || buf_len == 0) {
            break;
        }
        mutex_handle lock = 0;
        Socket* socket = socket_get_from_rdpsocket(sock, lock);
        if (!socket) {
            ret = RDPERROR_SOCKET_INVALIDSOCKET;
            break;
        }
        ret = socket->udp_send(ip, port, buf, buf_len);
        mutex_unlock(lock);
    } while (0);

    return  ret;
}
i32 core_addr_to(const sockaddr* addr, ui32 addrlen, char* ip, ui32* iplen, ui32* port)
{
    if (addr->sa_family == AF_INET) {
        if (addrlen != sizeof(sockaddr_in) || *iplen < sizeof(sockaddr_in)) {
            return RDPERROR_INVALIDPARAM;
        }
    }
    if (addr->sa_family == AF_INET6) {
        if (addrlen != sizeof(sockaddr_in6) || *iplen < sizeof(sockaddr_in6)) {
            return RDPERROR_INVALIDPARAM;
        }
    }
    return socket_api_addr_to(addr, ip, iplen, port);
}