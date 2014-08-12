#include "../include/rdp.h"
#include <memory.h>
#include <stdio.h>
#ifdef PLATFORM_OS_WINDOWS
#pragma comment(lib, "rdp.lib")
#endif


static void __cdecl on_connect(const rdp_on_connect_param* param)
{
    printf("on_connect  %d\n", param->err);
}
static void __cdecl on_accept(const rdp_on_accept_param* param)
{
    char ip[32] = { 0 };
    ui32 len = 32;
    ui32 port = 0;
    rdp_addr_to(param->addr, param->addrlen, ip, &len, &port);
    printf("on_accept   %s:%d\n", ip, port);
}
static void __cdecl on_disconnect(const rdp_on_disconnect_param* param)
{
    printf("on_disconnect %d\n", param->err);
}
static void __cdecl on_recv(const rdp_on_recv_param* param)
{
    //printf("on_recv %d, %d\n", param->err, buf_len);
}
static  void __cdecl on_send(const rdp_on_send_param* param)
{
    // printf("on_send %d\n", param->err);
}
static void __cdecl on_udp_recv(const rdp_on_udp_recv_param* param)
{

}

int server()
{
    rdp_startup_param startup_param = { 0 };
    startup_param.version = RDP_SDK_VERSION;
    startup_param.max_sock = 1;
    startup_param.recv_thread_num = 0;
    startup_param.recv_buf_size = 4 * 1024;
    i32 ret = rdp_startup(&startup_param);
    if (ret < 0) {
        return ret;
    }

    rdp_socket_create_param socket_create_param = { 0 };
    socket_create_param.is_v4 = true;
    socket_create_param.in_session_hash_size = 1024;
    socket_create_param.on_connect = on_connect;
    socket_create_param.on_accept = on_accept;
    socket_create_param.on_disconnect = on_disconnect;
    socket_create_param.on_recv = on_recv;
    socket_create_param.on_send = on_send;
    socket_create_param.on_udp_recv = on_udp_recv;
    RDPSOCKET sock = 0;
    ret = rdp_socket_create(&socket_create_param, &sock);
    if (ret < 0) {
        return ret;
    }
    ret = rdp_socket_bind(sock, "0.0.0.0", 9000);
    if (ret < 0) {
        return ret;
    }
    ret = rdp_socket_listen(sock);
    if (ret < 0) {
        return ret;
    }
    printf("server listen  0.0.0.0:9000\n");
    printf("press 'q' to quit\n");
    while (1) {
        if (startup_param.recv_thread_num != 0) {
            if ('q' == getchar()) {
                break;
            }
        }
        else{
            rdp_socket_recv(-1);
        }
    }
    rdp_socket_close(sock);
    rdp_cleanup();
    return 0;
}

int main(int argc, char* argv[])
{
    return server();
}
