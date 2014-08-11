#ifndef SESSION_H
#define SESSION_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "thread.h"
#include "timer.h"
#include "recv.h"
#include "protocol.h"
#include "socket_api.h"
#include "send_buffer.h"
#include <map>

typedef union sessionid {
    RDPSESSIONID sid;
    struct __sid {
        ui64 id : 32;
        ui64 unused : 22;
        ui64 is_in_come : 1;
        ui64 is_v4 : 1;
        ui64 socket_slot : 8;
    } _sid;
} sessionid;

typedef std::map<ui32, send_buffer*> send_buffer_list;

class SessionManager;
class Session{
public:
    Session();
    ~Session();
    void create(SessionManager* manager, RDPSESSIONID session_id, const sockaddr* addr);
    void destroy();
    ui8 get_state(){
        return state_;
    }
    void set_state(ui8 state){
        state_ = state;
    }
    sockaddr* get_addr(){
        return (sockaddr*)&addr_; 
    }
    RDPSESSIONID get_session_id(){
        return session_id_.sid;
    }
    i32 ctrl(ui16 cmd);
    i32 connect(ui32 timeout, const ui8* buf, ui16 buf_len);
    i32 disconnect(ui16 reason);
    i32 send(const ui8* buf, ui16 buf_len, ui32 flags);
    void on_recv(protocol_header* ph);
    void on_update(const timer_val& now);
protected:
    i32 ack(ui32* seq_num_ack, ui8 seq_num_ack_count);
    i32 ctrl_ack(ui32 seq_num_ack, ui16 cmd, ui16 error);
    i32 connect_ack(ui32 seq_num_ack, ui16 error);
    i32 heartbeat();

    void on_ack(protocol_ack* p);
    void on_ctrl(protocol_ctrl* p);
    void on_ctrl_ack(protocol_ctrl_ack* p);
    void on_connect(protocol_connect* p);
    void on_connect_ack(protocol_connect_ack* p);
    void on_disconnect(protocol_disconnect* p);
    void on_heartbeat(protocol_heartbeat* p);
    void on_data(protocol_data* p);
    void on_data_noack(protocol_data_noack* p);

    void on_handle_ack(ui32* seq_num_ack, ui8 seq_num_ack_count);
    void on_handle_ctrl(protocol_ctrl* p);
    void on_handle_ctrl_ack( protocol_ctrl_ack* p);
    void on_handle_connect( protocol_connect* p);
    void on_handle_connect_ack(protocol_connect_ack* p);
    void on_handle_disconnect(protocol_disconnect* p);
    void on_handle_heartbeat(protocol_heartbeat* p);
    void on_handle_data(protocol_data* p);
    void on_handle_data_noack( protocol_data_noack* p);
protected:
    SessionManager*   manager_;
    sessionid         session_id_; //
    i8                state_;      //RDPSTATUS
    sockaddr_in6      addr_;

    ui32              seq_num_;  //发送编号
    timer_val         recv_last_;//最后一次接收包时间,计算心跳  
    ui16              peer_window_size_;//对方接收窗口大小

    ui16              peer_ack_timerout_;//对方包确认超时

    send_buffer_list  send_buffer_list_;
};

inline bool session_is_in_come(RDPSESSIONID session_id)
{
    sessionid sid;
    sid.sid = session_id;
    return sid._sid.is_in_come == true;
}

#endif