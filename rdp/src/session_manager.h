#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "hash.h"
#include "thread.h"
#include "recv.h"
#include "socket_api.h"

#include "socket.h"


#define RDPSOCKETSTATUS_NONE 0
#define RDPSESSIONSTATUS_NONE 0

inline ui32 hash_key_id_ui32(ui32 const& key)
{
    return key;
}
inline bool hash_key_cmp_ui32(ui32 const& key1,  ui32 const& key2)
{
    return key1 == key2;
}
inline ui32 hash_key_id_sockaddr(sockaddr* const& key)
{
    ui32 len = 0;
    if (key->sa_family == AF_INET6) {
        len = sizeof(sockaddr_in6);
    } else {
        len = sizeof(sockaddr_in);
    }
    const rdp_startup_param& param = socket_get_startup_param();
    if (param.on_hash_addr){
        return param.on_hash_addr(key, len);
    }
    return socket_api_jenkins_one_at_a_time_hash((ui8 *)key, len);
}
inline bool hash_key_cmp_sockaddr(sockaddr* const& key1, sockaddr* const& key2)
{
    if (key1->sa_family != key2->sa_family) {
        return false;
    }
    ui32 len = 0;
    if (key1->sa_family == AF_INET6) {
        len = sizeof(sockaddr_in6);
    } else {
        len = sizeof(sockaddr_in);
    }
    return socket_api_addr_is_same(key1, key2, len);
}

class Session;
typedef Hash<ui32, Session*, hash_key_id_ui32, hash_key_cmp_ui32>     SessionIdList;
typedef Hash<sockaddr*, Session*, hash_key_id_sockaddr, hash_key_cmp_sockaddr> SessionAddrList;


class Socket;
class SessionManager
{
public:
    SessionManager();
    ~SessionManager();
    void create(Socket* socket, ui16 in_come_hash_size);
    void destroy();
    Socket* get_socket(){ return socket_; }
    Session* find(RDPSESSIONID session_id);
    Session* find(const sockaddr* addr);

    i32 connect(const char* ip, ui32 port, ui32 timeout, const ui8* buf, ui16 buf_len, RDPSESSIONID* session_id);
    i32 close(RDPSESSIONID session_id, ui16 reason, bool send_disconnect = true);
    i32 get_state(RDPSESSIONID session_id, ui32*state);
    i32 send(RDPSESSIONID session_id, const ui8* buf, ui16 buf_len, ui32 flags);
    i32 udp_send(sockaddr* addr, const ui8* buf, ui16 buf_len);
    void on_recv(thread_handle handle, recv_result* result);
    void on_update(thread_handle handle, const timer_val& now);
protected:
    Socket*               socket_;

    SessionIdList         in_come_session_id_list_;
    SessionAddrList       in_come_session_addr_list_;
    
    SessionIdList         out_come_session_id_list_;
    SessionAddrList       out_come_session_addr_list_;

    ui32                  in_come_id_;
    ui32                  out_come_id_;

    timer_val             last_update_;
    ui32                  last_in_come_update_hash_id_;
    ui32                  last_out_come_update_hash_id_;
};




#endif