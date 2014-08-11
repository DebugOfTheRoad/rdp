#ifndef SOCKETAPI_H
#define SOCKETAPI_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "buffer.h"

#ifdef PLATFORM_OS_WINDOWS
#include <ws2tcpip.h>
#include <winsock2.h>

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#else
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#endif

#if !defined (_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

typedef ui64 addrhash;

i32 socket_api_startup(ui32& max_udp_dg);
i32 socket_api_cleanup();
SOCKET socket_api_create(bool v4);
i32 socket_api_bind(SOCKET s,
                    sockaddr * addr,
                    i32  addrlen);
i32 socket_api_recvfrom(SOCKET s,
                        buffer& buf,
                        i32 flags,
                        sockaddr  * from,
                        i32  * fromlen);

i32 socket_api_sendto(SOCKET s,
                      const buffer& buf,
                      int flags,
                      const sockaddr *to,
                      int tolen);

i32 socket_api_setsockopt(SOCKET s,
                          i32 level,
                          i32 optname,
                          const char * optval,
                          i32 optlen);

i32 socket_api_close(SOCKET s);

i32 socket_api_getsyserror();


sockaddr* socket_api_addr_create(bool v4, const sockaddr *addr);
sockaddr* socket_api_addr_create(const sockaddr *addr);
void socket_api_addr_destroy(sockaddr *addr);
bool socket_api_addr_is_v4(const sockaddr *addr);
i32  socket_api_addr_len(const sockaddr *addr);
i32 socket_api_addr_from(const char* ip, ui32 port, sockaddr *addrto, bool* is_anyaddr = 0);
i32 socket_api_addr_to(const sockaddr* addrfrom, char* ip, ui32* iplen, ui32* port);
bool socket_api_addr_is_same(const sockaddr *addr1, const sockaddr *addr2, ui32 addrlen);
 
ui32 socket_api_jenkins_one_at_a_time_hash(ui8 *key, ui32 len);

#endif