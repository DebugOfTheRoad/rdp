#include "socket_api.h"
#include "alloc.h"

i32 socket_api_startup(ui32& max_udp_dg)
{
    i32 ret = RDPERROR_SUCCESS;
    max_udp_dg = 0;
#if defined PLATFORM_OS_WINDOWS
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData = {0};
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (0 != err) {
        SetLastError(err);
        ret = RDPERROR_SYSERROR;
    } else {
        max_udp_dg = wsaData.iMaxUdpDg;
    }
#endif
    return ret;
}
i32 socket_api_cleanup()
{
    i32 ret = RDPERROR_SUCCESS;
#if defined PLATFORM_OS_WINDOWS
    int err = WSACleanup();
    if (err == SOCKET_ERROR) {
        ret = RDPERROR_SYSERROR;
    }
#endif
    return ret;
}
SOCKET socket_api_create(bool v4)
{
    return ::socket(v4 ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
}
i32 socket_api_bind(SOCKET s,
                    sockaddr * addr,
                    i32  addrlen)
{
    return ::bind(s, addr, addrlen);
}
i32 socket_api_recvfrom(SOCKET s,
                        buffer& buf,
                        i32 flags,
                        sockaddr * from,
                        i32 * fromlen)
{
    buf.length = buf.capcity;
    int ret = ::recvfrom(s, (char*)buf.ptr, buf.capcity, flags, from, fromlen);
    return ret;
}
i32 socket_api_sendto(SOCKET s,
                      const buffer& buf,
                      i32 flags,
                      const sockaddr *to,
                      i32 tolen)
{
    return ::sendto(s, (const char*)buf.ptr, buf.length, flags, to, tolen);
}
i32 socket_api_setsockopt(SOCKET s,
                          i32 level,
                          i32 optname,
                          const char  * optval,
                          i32 optlen)
{
    return ::setsockopt(s, level, optname, optval, optlen);
}
i32 socket_api_close(SOCKET s)
{
    shutdown(s, SD_BOTH);
#ifndef PLATFORM_OS_WINDOWS
    ::close(s);
#else
    ::closesocket(s);
#endif
    return 0;
}
i32 socket_api_getsyserror()
{
#ifndef PLATFORM_OS_WINDOWS
    return errno;
#else
    return WSAGetLastError();
#endif
}
sockaddr* socket_api_addr_create(bool v4, const sockaddr *addr)
{
    sockaddr* add = 0;
    if (v4) {
        add = (sockaddr*)alloc_new_object<sockaddr_in>();
        memset(add, 0, sizeof(sockaddr_in));
    } else {
        add = (sockaddr*)alloc_new_object<sockaddr_in6>();
        memset(add, 0, sizeof(sockaddr_in6));
    }
    if (addr) {
        memcpy(add, addr, v4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
    }
    add->sa_family = v4 ? AF_INET : AF_INET6;
    return add;
}
sockaddr* socket_api_addr_create(const sockaddr *addr)
{
    if (!addr) {
        return 0;
    }
    bool v4 = socket_api_addr_is_v4(addr);
    return socket_api_addr_create(v4, addr);
}
void socket_api_addr_destroy(sockaddr *addr)
{
    if (!addr) {
        return ;
    }
    bool v4 = socket_api_addr_is_v4(addr);
    if (v4) {
        alloc_delete_object((sockaddr_in*)addr);
    } else {
        alloc_delete_object((sockaddr_in6*)addr);
    }
}
bool socket_api_addr_is_v4(const sockaddr *addr)
{
    if (!addr) {
        return false;
    }
    return addr->sa_family == AF_INET;
}
i32  socket_api_addr_len(const sockaddr *addr)
{
    if (!addr) {
        return 0;
    }
    if (addr->sa_family == AF_INET) {
        return sizeof(sockaddr_in);
    }
    if (addr->sa_family == AF_INET6) {
        return sizeof(sockaddr_in6);
    }
    return 0;
}
i32 socket_api_addr_from(const char* ip, ui32 port, sockaddr *addrto, bool* is_anyaddr)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!addrto) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        bool v4 = socket_api_addr_is_v4(addrto);

        char realip[256] = { 0 };
        if (0 == ip || '\0' == *ip || 0 == *ip) {
            if (v4) {
                strcpy_s(realip, 256, "0.0.0.0");
            } else {
                strcpy_s(realip, 256, "::");
            }
        } else {
            strcpy_s(realip, 256, ip);
        }
        if (is_anyaddr) {
            if (v4) {
                *is_anyaddr = 0 == strcmp(realip, "0.0.0.0");
            } else {
                *is_anyaddr = 0 == strcmp(realip, "::");
            }
        }

        if (port == 0) {
            port = INADDR_ANY;
        }

        if (v4) {
            sockaddr_in* addr = (sockaddr_in*)addrto;
            addr->sin_family = AF_INET;
            addr->sin_port = htons(port);
            if (0 >= inet_pton(addr->sin_family, realip, &addr->sin_addr)) {
                ret = RDPERROR_SYSERROR;
                break;
            }
        } else {
            sockaddr_in6* addr = (sockaddr_in6*)addrto;
            addr->sin6_family = AF_INET6;
            addr->sin6_port = htons(port);
            if (0 >= inet_pton(addr->sin6_family, realip, &addr->sin6_addr)) {
                ret = RDPERROR_SYSERROR;
                break;
            }
        }
    } while (0);
    return ret;
}
i32 socket_api_addr_to(const sockaddr* addrfrom, char* ip, ui32* iplen, ui32* port)
{
    i32 ret = RDPERROR_SUCCESS;
    do {
        if (!addrfrom || !ip || !iplen) {
            ret = RDPERROR_INVALIDPARAM;
            break;
        }
        bool v4 = socket_api_addr_is_v4(addrfrom);
        if (v4) {
            sockaddr_in* addr = (sockaddr_in*)addrfrom;
            addr->sin_family = AF_INET;
            if (port) {
                *port = ntohs(addr->sin_port);
            }
            if (0 >= inet_ntop(addr->sin_family, &addr->sin_addr, ip, *iplen)) {
                ret = RDPERROR_SYSERROR;
                break;
            }
            *iplen = strlen(ip);
        } else {
            sockaddr_in6* addr = (sockaddr_in6*)addrfrom;
            addr->sin6_family = AF_INET6;
            if (port) {
                *port = ntohs(addr->sin6_port);
            }
            if (0 >= inet_ntop(addr->sin6_family, &addr->sin6_addr, ip, *iplen)) {
                ret = RDPERROR_SYSERROR;
                break;
            }
            *iplen = strlen(ip);
        }
    } while (0);
    return ret;
}
bool socket_api_addr_is_same(const sockaddr *addr1, const sockaddr *addr2, ui32 addrlen)
{
    ui8* caddr1 = (ui8*)addr1;
    ui8* caddr2 = (ui8*)addr2;
    for (ui32 i = 0; i < addrlen; ++i)
    {
        if (*caddr1++ != *caddr2++)
        {
            return false;
        }
    }
    return true;
}
ui32 socket_api_jenkins_one_at_a_time_hash(ui8 *key, ui32 len)
{
    ui32 hash, i;
    for (hash = i = 0; i < len; ++i) {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

 