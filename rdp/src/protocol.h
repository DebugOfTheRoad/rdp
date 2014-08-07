#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"

#define IP_HEAD_SIZE 20
#define UDP_HEAD_SIZE 8

#define RDP_PROTOCOL 1

typedef enum protocol_ids {
    //����ack��(proto_*_ack),������Ҫȷ��,δ�յ�ack��һ��,�ط�����
    proto_ack = 0,         //ȷ�ϰ� c->s ; s->c;
    proto_ctrl,            //���ư�c->s ;s->c , s��proto_ctrl_ack��Ӧ
    proto_ctrl_ack,        //����ȷ�ϰ�c->s ;s->c
    proto_connect,         //���������c->s, s��proto_connect_ack��Ӧ
    proto_connect_ack,     //����ȷ�ϰ� s->c
    proto_disconnect,      //������c->s ; s->c
    proto_heartbeat,       //������c->s,s��proto_ack��Ӧ
    proto_data,            //���ݰ�c->s ; s->c
    proto_udp_data,           //���ݰ�,���������ӵ����ݰ�
    proto_end,
} protocol_ids;

//Э��ͷ
typedef struct protocol_header {
    ui8  protocol;    //RDP_PROTOCOL
    ui8  protocol_id; //Э��id
    ui32 seq_num;     //���к�
    
    protocol_header(ui8  proto_id) {
        protocol = RDP_PROTOCOL;
        protocol_id = proto_id;
        
        seq_num = 0;
    }
} protocol_header;

typedef struct protocol_ack : public protocol_header {
    ui32  win_size;    //�������ڴ�С
    ui8   seq_num_ack_count;    //���к�ȷ�Ϻ�����
    //ui32* seq_num_ack;
    protocol_ack()
        :protocol_header(proto_ack) {
        win_size = 0;
        seq_num_ack_count = 0;
    }
} protocol_ack;

typedef struct protocol_ctrl : public protocol_header {
    ui16 cmd;         //����

    protocol_ctrl()
        :protocol_header(proto_ctrl) {
        cmd = 0;
    }
} protocol_ctrl;

typedef struct protocol_ctrl_ack : public protocol_header {
    ui32 win_size;    //�������ڴ�С
    ui32 seq_num_ack; //���к�ȷ�Ϻ�
    ui16 cmd;         //����
    ui16 error;       //����ctrl_ack_error

    typedef enum ctrl_ack_error {
        ctrl_ack_success = 0, //����ɹ�

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
    ui16 data_size; //���ݴ�С
    //ui8* data; 
    protocol_connect()
        :protocol_header(proto_connect) {
        data_size = 0;
    }
} protocol_connect;

typedef struct protocol_connect_ack : public protocol_header {
    ui32 win_size;    //�������ڴ�С
    ui32 seq_num_ack; //���к�ȷ�Ϻ�
    ui16 error;                    //connect_ack_error
    ui16 heart_beat_timeout;     //������ʱ s

    typedef enum connect_ack_error {
        connect_ack_success = 0, //�������ӳɹ�
        connect_ack_deny_income, //������δ���ڼ���״̬
        connect_ack_busy,        //������æ
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
    ui32 seq_num_ack; //���к�ȷ�Ϻ�
    ui16 ack_timeout; //ȷ�ϳ�ʱ
    ui16 frag_num; //��Ƭ��
    ui16 frag_offset; //��Ƭƫ��
    ui16 data_size; //���ݴ�С
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
    ui16 data_size; //���ݴ�С
    //ui8* data;
    protocol_udp_data()
        :protocol_header(proto_udp_data) {
        data_size = 0;
    }
} protocol_udp_data;



#endif