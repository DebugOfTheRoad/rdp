#include "../include/rdp.h"
 
#include <memory.h>
#include <stdio.h>
#ifdef PLATFORM_OS_WINDOWS
#include <windows.h>
#pragma comment(lib, "rdp.lib")
#endif

RDPSESSIONID sid = 0;

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
    sid = 0;
    printf("on_disconnect %d\n", param->err);
}
static void __cdecl on_recv(const rdp_on_recv_param* param)
{
    //printf("on_recv %d, %d\n", param->err, param->buf_len);
}
static  void __cdecl on_send(const rdp_on_send_param* param)
{
    if (param->err != 0)
    {
        printf("on_send %d\n", param->err);
    }
    
}
static void __cdecl on_udp_recv(const rdp_on_udp_recv_param* param)
{

}
int client()
{
    rdp_startup_param startup_param = { 0 };
    startup_param.version = RDP_SDK_VERSION;
    startup_param.max_sock = 1;
    startup_param.recv_thread_num = 1;
    startup_param.recv_buf_size = 4 * 1024;
    
    i32 ret = rdp_startup(&startup_param);
    if (ret < 0) {
        return ret;
    }

    rdp_socket_create_param socket_create_param = { 0 };
    socket_create_param.is_v4 = true;
    socket_create_param.in_session_hash_size = 1;
    socket_create_param.heart_beat_timeout = 60;
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
    printf("press 'd' to disconnect\n");
    printf("press 's' to send data to 127.0.0.1:9000\n");
    printf("press 'u' to send udp data to 157.4.0.1:9000\n");
    
    //ret = rdp_socket_connect(sock, "127.0.0.1", 9000, 1000, 0, 0, &sid);
    while (1) {
        char c = getchar();
        if ('q' == c) {
            break;
        } else if ('c' == c) {
            char* hi = "hi,wallee";
            //FIX ip 地址为 127.*的时候,在release版下会崩溃
            ret = rdp_socket_connect(sock, "127.0.0.1", 9000, 3000, (ui8*)hi, strlen(hi), &sid);
            if (ret < 0) {
                printf("rdp_socket_connect failed %d\n", ret);
            }
        } else if ('d' == c){
            ret = rdp_session_close(sock, sid, 0);
            sid = 0;
            if (ret < 0) {
                printf("rdp_session_close failed %d\n", ret);
            }
        } else if ('s' == c) {
            if (sid == 0) {
                printf("rdp_session_send failed :not connected\n");
                continue;
            }
            int times = 20*1024;
            int send = 0;
            int len = 1024;
            char* p = new char[len];
            while (send < times) {
                ret = rdp_session_send(sock, sid, (const ui8*)p, len, RDPSESSIONSENDFLAG_ACK);
                if (ret < 0) {
                    printf("rdp_session_send failed %d\n", ret);
                    break;
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
                ret = rdp_udp_send(sock, "10.0.0.1", 9000, (const ui8*)p, len);
                DWORD dw2 = GetTickCount();
                if (ret < 0) {
                    printf("rdp_udp_send failed %d\n", ret);
                    break;
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