#ifndef SOCKETS_H
#define SOCKETS_H
 
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "socket_api.h"
#include "thread.h"

typedef struct socket_info{
    //comment begin 下面注视部分数据，运行过程中不会发生变化，不需要使用socket_info锁
    rdp_socket_create_param create_param;

    mutex_handle      lock;
    mutex_cond_handle cond;

    sockaddr*         addr; //local addr
    //comment end

    SOCKET            sock;//需要使用socket_info加锁
    ui8               state;//需要使用socket_info加锁
    ui32              window_size;
}socket_info;
 
typedef void* socket_handle;

extern ui32 s_max_udp_dg ;
extern rdp_startup_param s_socket_startup_param;

socket_handle RDPSOCKET2socket_handle(RDPSOCKET sock);
RDPSOCKET socket_handle2RDPSOCKET(socket_handle handle);
socket_info* socket_get_socket_info(SOCKET sock);
socket_info* socket_get_socket_info(socket_handle handle);

i32 socket_startup(rdp_startup_param* param);
i32 socket_cleanup();
i32 socket_startup_get_param(rdp_startup_param* param);
i32 socket_create(rdp_socket_create_param* param, socket_handle* handle);
i32 socket_get_create_param(socket_handle handle, rdp_socket_create_param* param);
i32 socket_get_state(socket_handle handle, ui32* state);
i32 socket_destroy(socket_handle handle);
i32 socket_bind(socket_handle handle, const char* ip, ui32 port);
i32 socket_listen(socket_handle handle);
i32 socket_connect(socket_handle handle, const char* ip, ui32 port, ui32 timeout, 
    RDPSESSIONID* session_id, const ui8* buf, ui16 buf_len);

i32 socket_getsyserror();
i32 socket_getsyserrordesc(i32 err, char* desc, ui32 desc_len);
i32 socket_session_close(socket_handle handle, RDPSESSIONID session_id);
i32 socket_session_get_state(socket_handle handle, RDPSESSIONID session_id, ui32* state);
i32 socket_session_send(socket_handle handle, RDPSESSIONID session_id, const ui8* buf, ui16 buf_len,
    ui32 flags,
    ui32* local_send_queue_size, ui32* peer_unused_recv_queue_size);
bool socket_session_is_income(RDPSESSIONID session_id);

i32 socket_udp_send(socket_handle handle, const char* ip, ui32 port, const ui8* buf, ui16 buf_len);

i32 socket_addr_to(const sockaddr* addr, ui32 addrlen, char* ip, ui32 iplen, ui32* port);

#ifdef PLATFORM_CONFIG_TEST
void socket_test();
#endif


#endif