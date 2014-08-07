#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"

#define IP_HEAD_SIZE 20
#define UDP_HEAD_SIZE 8

#define RDP_PROTOCOL 1

typedef enum protocol_ids {
    //对于ack包(proto_*_ack),都不需要确认,未收到ack的一方,重发请求
    proto_ack = 0,         //确认包 c->s ; s->c;
    proto_ctrl,            //控制包c->s ;s->c , s用proto_ctrl_ack响应
    proto_ctrl_ack,        //控制确认包c->s ;s->c
    proto_connect,         //连接请求包c->s, s用proto_connect_ack响应
    proto_connect_ack,     //连接确认包 s->c
    proto_disconnect,      //断连包c->s ; s->c
    proto_heartbeat,       //心跳包c->s,s用proto_ack响应
    proto_data,            //数据包c->s ; s->c
    proto_udp_data,           //数据包,不基于连接的数据包
    proto_end,
} protocol_ids;

//协议头
typedef struct protocol_header {
    ui8  protocol;    //RDP_PROTOCOL
    ui8  protocol_id; //协议id
    ui32 seq_num;     //序列号
    
    protocol_header(ui8  proto_id) {
        protocol = RDP_PROTOCOL;
        protocol_id = proto_id;
        
        seq_num = 0;
    }
} protocol_header;

typedef struct protocol_ack : public protocol_header {
    ui32  win_size;    //滑动窗口大小
    ui8   seq_num_ack_count;    //序列号确认号数量
    //ui32* seq_num_ack;
    protocol_ack()
        :protocol_header(proto_ack) {
        win_size = 0;
        seq_num_ack_count = 0;
    }
} protocol_ack;

typedef struct protocol_ctrl : public protocol_header {
    ui16 cmd;         //命令

    protocol_ctrl()
        :protocol_header(proto_ctrl) {
        cmd = 0;
    }
} protocol_ctrl;

typedef struct protocol_ctrl_ack : public protocol_header {
    ui32 win_size;    //滑动窗口大小
    ui32 seq_num_ack; //序列号确认号
    ui16 cmd;         //命令
    ui16 error;       //错误ctrl_ack_error

    typedef enum ctrl_ack_error {
        ctrl_ack_success = 0, //命令成功

    } ctrl_ack_error;

    protocol_ctrl_ack()
        :protocol_header(proto_ctrl_ack) {
        win_size = 0;
        seq_num_ack = 0;
        cmd = 0;
        error = ctrl_ack_success;
    }
} protocol_ctrl_ack;

typedef struct protocol_connect : public protocol_header {
    ui16 data_size; //数据大小
    //ui8* data; 
    protocol_connect()
        :protocol_header(proto_connect) {
        data_size = 0;
    }
} protocol_connect;

typedef struct protocol_connect_ack : public protocol_header {
    ui32 win_size;    //滑动窗口大小
    ui32 seq_num_ack; //序列号确认号
    ui16 error;                    //connect_ack_error
    ui16 heart_beat_timeout;     //心跳超时 s

    typedef enum connect_ack_error {
        connect_ack_success = 0, //建立连接成功
        connect_ack_deny_income, //服务器未处于监听状态
        connect_ack_busy,        //服务器忙
    } connect_ack_error;

    protocol_connect_ack()
        :protocol_header(proto_connect_ack) {
        win_size = 0;
        seq_num_ack = 0;
        error = connect_ack_success;
        heart_beat_timeout = 0;
    }
} protocol_connect_ack;

typedef struct protocol_disconnect : public protocol_header {
    ui16 reason;

    typedef enum disconnect_reason {
        disconnect_reason_none = 0, //
    } disconnect_reason;

    protocol_disconnect()
        :protocol_header(proto_disconnect) {
        reason = 0;
    }
} protocol_disconnect;

typedef struct protocol_heartbeat : public protocol_header {

    protocol_heartbeat()
        :protocol_header(proto_heartbeat) {
    }
} protocol_heartbeat;

typedef struct protocol_data : public protocol_header {
    ui32 seq_num_ack; //序列号确认号
    ui16 ack_timeout; //确认超时
    ui16 frag_num; //分片号
    ui16 frag_offset; //分片偏移
    ui16 data_size; //数据大小
    //ui8* data;

    protocol_data()
        :protocol_header(proto_data) {
        seq_num_ack = 0;
        ack_timeout = 0;
        frag_num = 0;
        frag_offset = 0;
        data_size = 0;
    }
} protocol_data;

typedef struct protocol_udp_data : public protocol_header {
    ui16 data_size; //数据大小
    //ui8* data;
    protocol_udp_data()
        :protocol_header(proto_udp_data) {
        data_size = 0;
    }
} protocol_udp_data;



#endif