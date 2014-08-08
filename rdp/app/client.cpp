#include "../include/rdp.h"
 
#include <memory.h>
#include <stdio.h>
#ifdef PLATFORM_OS_WINDOWS
#include <windows.h>
#pragma comment(lib, "rdp.lib")
#endif

static void __cdecl on_connect(const rdp_on_connect_param& param)
{
    printf("on_connect  %d\n", param.err);
}
static void __cdecl on_accept(const rdp_on_accept& param)
{
    char ip[32] = { 0 };
    ui32 port = 0;
    rdp_addr_to(param.addr, param.addrlen, ip, 32, &port);
    printf("on_accept   %s:%d\n", ip, port);
}
static void __cdecl on_disconnect(const rdp_on_disconnect_param& param)
{
    printf("on_disconnect %d\n", param.err);
}
static void __cdecl on_recv(const rdp_on_recv_param& param)
{
    //printf("on_recv %d, %d\n", err, buf_len);
}
static  void __cdecl on_send(const rdp_on_send_param& param)
{
   // printf("on_send %d, %d, %d\n", err, local_send_queue_size, peer_unused_recv_queue_size);
}
static void __cdecl on_udp_recv(const rdp_on_udp_recv_param& param)
{

}
int client()
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
    startup_param.on_udp_recv = on_udp_recv;
    i32 ret = rdp_startup(&startup_param);
    if (ret < 0) {
        return ret;
    }

    rdp_socket_create_param socket_create_param = { 0 };
    socket_create_param.v4 = true;
    socket_create_param.in_session_hash_size = 1;
    socket_create_param.out_session_hash_size = 1;
    
    RDPSOCKET sock = 0;
    ret = rdp_socket_create(&socket_create_param, &sock);
    if (ret < 0) {
        return ret;
    }
    ret = rdp_socket_bind(sock, 0, 0);
    if (ret < 0) {
        return ret;
    }
    /*  ret = rdp_socket_listen(sock, 10);
     if (ret < 0) {
         return ret;
     }
     printf("client listen  0.0.0.0:9001\n");*/
    //ret = rdp_udp_send(sock, "127.0.0.1", 9000, (const ui8*)"sdf", 2);
    printf("press 'q' to quit\n");
    printf("press 'c' to connect 127.0.0.1:9000\n");
    printf("press 's' to send data to 127.0.0.1:9000\n");
    printf("press 'u' to send udp data to 157.4.0.1:9000\n");
    RDPSESSIONID sid = 0;
    while (1) {
        char c = getchar();
        if ('q' == c) {
            break;
        } else if ('c' == c) {
            //FIX ip 地址为 127.*的时候,在release版下会崩溃
            ret = rdp_socket_connect(sock, "127.0.0.1", 9000, 3000, &sid);
            if (ret < 0) {
                printf("conntect failed %d\n", ret);
            }
        } else if ('s' == c) {
            if (sid == 0) {
                printf("send failed :not connected\n");
                continue;
            }
            int times = 1024*1024;
            int send = 0;
            int len = 1024;
            char* p = new char[len];
            while (send < times) {
                ui32 local_send_queue_size = 0;
                ui32 peer_unused_recv_queue_size = 0;
                ret = rdp_session_send(sock, sid, (const ui8*)p, len, RDPSESSIONSENDFLAG_ACK,
                    &local_send_queue_size, &peer_unused_recv_queue_size);
                if (ret < 0) {
                    printf("send failed %d\n", ret);
                    break;
                }
                if (local_send_queue_size > 1024*1024){
#ifdef PLATFORM_OS_WINDOWS
                    Sleep(1);
#else
                    sleep(1000);
#endif   
                }
                send++;
            }
            delete p;
        }
        else if (c == 'u')
        {
            int times = 1024 * 1024;
            int send = 0;
            int len = 1024;
            char* p = new char[len];
            while (send < times) {
                ui32 local_send_queue_size = 0;
                ui32 peer_unused_recv_queue_size = 0;
                DWORD dw1 = GetTickCount();
                ret = rdp_udp_send(sock, "157.4.0.1", 9000, (const ui8*)p, len);
                DWORD dw2 = GetTickCount();
                if (ret < 0) {
                    printf("send failed %d\n", ret);
                    break;
                }
                else{
                    printf("send udp %d : %d\n", ret, dw2 - dw1);
                }
                if (local_send_queue_size > 1024 * 1024){
#ifdef PLATFORM_OS_WINDOWS
                    Sleep(1);
#else
                    sleep(1000);
#endif   
                }
                send++;
            }
            delete p;
        }
    }
    rdp_socket_close(sock);
    rdp_cleanup();
    return 0;
}

int main(int argc, char* argv[])
{
    return client();
}