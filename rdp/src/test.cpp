#include "test.h"
#include "../include/rdp.h"
#include "timer.h"
#include "thread.h"
#include "socket.h"

#ifdef PLATFORM_CONFIG_TEST


static void __cdecl on_connect(const rdp_on_connect_param& param)
{
     
}
static void __cdecl on_accept(const rdp_on_accept& param)
{
    char ip[32] = { 0 };
    ui32 port = 0;
    rdp_addr_to(param.addr, param.addrlen, ip, 32, &port);
     
}
static void __cdecl on_disconnect(const rdp_on_disconnect_param& param)
{
     
}
static void __cdecl on_recv(const rdp_on_recv_param& param)
{
     
}
static  void __cdecl on_send(const rdp_on_send_param& param)
{
    
}

void test_begin()
{
    rdp_startup_param startup_param = { 0 };
    startup_param.version = RDP_SDK_VERSION;
    startup_param.max_sock = 1;
    startup_param.recv_thread_num = 1;
    startup_param.recv_buf_size = 4 * 1024;

    startup_param.on_connect = on_connect;
    startup_param.on_accept = on_accept;
    startup_param.on_disconnect = on_disconnect;
    startup_param.on_recv = on_recv;
    startup_param.on_send = on_send;
    i32 ret = rdp_startup(&startup_param);
    if (ret < 0) {
        return ;
    }

    timer_test();
    thread_test();
    socket_test();
}
void test_end()
{
    rdp_cleanup();
}

#endif
