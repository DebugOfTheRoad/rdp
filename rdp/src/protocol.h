#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"

#define IP_HEAD_SIZE 20
#define UDP_HEAD_SIZE 8

#define CHECKSUM_SEED 0x34

typedef enum protocol_ids {
    //对于ack包(proto_*_ack),都不需要确认,未收到ack的一方,重发请求
    proto_ack = 0,         //确认包 c->s ; s->c;
    proto_ctrl,            //控制包c->s ;s->c , s用proto_ctrl_ack确认
    proto_ctrl_ack,        //控制确认包c->s ;s->c
    proto_connect,         //连接请求包c->s, s用proto_connect_ack确认
    proto_connect_ack,     //连接确认包 s->c
    proto_disconnect,      //断连包c->s ; s->c,用proto_ack确认
    proto_heartbeat,       //心跳包c->s,s用proto_ack确认
    proto_data,            //数据包c->s ; s->c,用proto_ack确认
    proto_data_noack,      //数据包c->s ; s->c,不需要确认的数据包

    proto_udp_data,        //不基于连接的udp数据包
    proto_end,
} protocol_ids;

//协议头
typedef struct protocol_header {
    ui8 check_sum;              //头部校验码
    ui8 protocol;               //协议id

    protocol_header(ui8  proto_id) {
        check_sum = 0;
        protocol = proto_id;
    }
} protocol_header;

typedef struct protocol_ack : public protocol_header {
    ui16  win_size;    //滑动窗口大小
    ui8   seq_num_ack_count;    //编号确认号数量
    //ui32* seq_num_ack;
    protocol_ack()
        :protocol_header(proto_ack) {
        win_size = 0;
        seq_num_ack_count = 0;
    }
} protocol_ack;

typedef struct protocol_ctrl : public protocol_header {
    ui32 seq_num;     //编号
    ui16 cmd;         //命令

    protocol_ctrl()
        :protocol_header(proto_ctrl) {
        seq_num = 0;
        cmd = 0;
    }
} protocol_ctrl;

typedef struct protocol_ctrl_ack : public protocol_header {
    ui32 seq_num_ack; //编号确认号
    ui16 cmd;         //命令
    ui16 error;       //错误ctrl_ack_error

    typedef enum ctrl_ack_error {
        ctrl_ack_success = 0, //命令成功

    } ctrl_ack_error;

    protocol_ctrl_ack()
        :protocol_header(proto_ctrl_ack) {
        seq_num_ack = 0;
        cmd = 0;
        error = ctrl_ack_success;
    }
} protocol_ctrl_ack;

typedef struct protocol_connect : public protocol_header {
    ui32 seq_num;     //编号
    ui16 ack_timeout; //确认超时
    ui16 data_size; //数据大小
    //ui8* data;

    protocol_connect()
        :protocol_header(proto_connect) {
        seq_num = 0;
        ack_timeout = 0;
        data_size = 0;
    }
} protocol_connect;

typedef struct protocol_connect_ack : public protocol_header {
    ui32 seq_num_ack; //编号确认号
    ui16 win_size;    //滑动窗口大小
    ui16 error;       //connect_ack_error
    ui16 ack_timeout; //确认超时
    ui16 heart_beat_timeout;     //心跳超时 s

    typedef enum connect_ack_error {
        connect_ack_success = 0, //建立连接成功
        connect_ack_deny_income, //服务器未处于监听状态
        connect_ack_busy,        //服务器忙
        connect_ack_toomuch,     //达到连接数上限
    } connect_ack_error;

    protocol_connect_ack()
        :protocol_header(proto_connect_ack) {
        seq_num_ack = 0;
        win_size = 0;
        error = connect_ack_success;
        ack_timeout = 0;
        heart_beat_timeout = 0;
    }
} protocol_connect_ack;

typedef struct protocol_disconnect : public protocol_header {
    ui32 seq_num;     //编号
    ui16 reason;
    ui8  need_ack;

    typedef enum disconnect_reason {
        disconnect_reason_none = 0, //
    } disconnect_reason;

    protocol_disconnect()
        :protocol_header(proto_disconnect) {
        seq_num = 0;
        reason = 0;
        need_ack = false;
    }
} protocol_disconnect;

typedef struct protocol_heartbeat : public protocol_header {
    ui32 seq_num;     //编号

    protocol_heartbeat()
        :protocol_header(proto_heartbeat) {
        seq_num = 0;
    }
} protocol_heartbeat;

typedef struct protocol_data : public protocol_header {
    ui32 seq_num;     //编号
    ui32 seq_num_ack; //编号确认号
    ui8  flags;       //标志位
    ui16 data_size;   //数据大小
    //ui8* data;

    typedef enum disconnect_reason {
        data_ack_now = 0x01, //
    } disconnect_reason;

    protocol_data()
        :protocol_header(proto_data) {
        seq_num = 0;
        seq_num_ack = 0;
        flags = 0;
        data_size = 0;
    }
} protocol_data;

typedef struct protocol_data_noack : public protocol_header {

    ui16 data_size; //数据大小
    //ui8* data;

    protocol_data_noack()
        :protocol_header(proto_data_noack) {

        data_size = 0;
    }
} protocol_data_noack;

typedef struct protocol_udp_data : public protocol_header {
    ui16 data_size; //数据大小
    //ui8* data;

    protocol_udp_data()
        :protocol_header(proto_udp_data) {
        data_size = 0;
    }
} protocol_udp_data;

bool protocol_set_header(protocol_header* header);
bool protocol_check_header(protocol_header* header, ui16 size);
 
#endif