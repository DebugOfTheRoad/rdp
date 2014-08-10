#ifndef SOCKET_H
#define SOCKET_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "socket_api.h"
#include "thread.h"
#include "send_buffer.h"
#include "recv.h"


class SessionManager;
class Socket
{
public:
    Socket();
    ~Socket();
    const rdp_socket_create_param& get_create_param() {
        return create_param_;
    }
    void startup(ui8 slot);
    void cleanup();
    i32 create(rdp_socket_create_param* param);
    void destroy();
    ui8 get_slot() {
        return slot_;
    }
    mutex_handle get_lock(){
        return lock_;
    }
    SOCKET get_socket() {
        return socket_;
    }
    ui8 get_state(){
        return state_;
    }
    sockaddr* get_addr(){
        return addr_;
    }
    RDPSOCKET get_rdpsocket();
    i32 bind(const char* ip, ui32 port);
    i32 listen();
    i32 connect(const char* ip, ui32 port, ui32 timeout, const ui8* buf, ui16 buf_len , RDPSESSIONID* session_id);
    i32 session_close(RDPSESSIONID session_id, i32 reason);
    i32 session_get_state(RDPSESSIONID session_id, ui32* state);
    i32 session_send(RDPSESSIONID session_id, const ui8* buf, ui16 buf_len, ui32 flags);
    i32 udp_send(const char* ip, ui32 port, const ui8* buf, ui16 buf_len);
 
    void on_recv(thread_handle handle, recv_result* result);
    void on_update(thread_handle handle, const timer_val& now);
    i32 send(send_buffer* send_buf);
protected:
    rdp_socket_create_param create_param_;
    ui8                     slot_;
    mutex_handle            lock_;
    ui8                     state_;
    SOCKET                  socket_;
    sockaddr*               addr_;
    SessionManager*         sm_;
};
const rdp_startup_param& socket_get_startup_param();

Socket* socket_get_from_socket(SOCKET sock, mutex_handle& lock);
Socket* socket_get_from_rdpsocket(RDPSOCKET sock, mutex_handle& lock);

i32 socket_startup(rdp_startup_param* param);
i32 socket_cleanup();
i32 socket_startup_get_param(rdp_startup_param* param);
i32 socket_create(rdp_socket_create_param* param, Socket** socket, mutex_handle& lock_out);


#ifdef PLATFORM_CONFIG_TEST
void socket_test();
#endif


#endif