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
    //����ack��(proto_*_ack),������Ҫȷ��,δ�յ�ack��һ��,�ط�����
    proto_ack = 0,         //ȷ�ϰ� c->s ; s->c;
    proto_ctrl,            //���ư�c->s ;s->c , s��proto_ctrl_ackȷ��
    proto_ctrl_ack,        //����ȷ�ϰ�c->s ;s->c
    proto_connect,         //���������c->s, s��proto_connect_ackȷ��
    proto_connect_ack,     //����ȷ�ϰ� s->c
    proto_disconnect,      //������c->s ; s->c,��proto_ackȷ��
    proto_heartbeat,       //������c->s,s��proto_ackȷ��
    proto_data,            //���ݰ�c->s ; s->c,��proto_ackȷ��
    proto_data_noack,      //���ݰ�c->s ; s->c,����Ҫȷ�ϵ����ݰ�

    proto_udp_data,        //���������ӵ�udp���ݰ�
    proto_end,
} protocol_ids;

//Э��ͷ
typedef struct protocol_header {
    ui8 check_sum;              //ͷ��У����
    ui8 protocol;               //Э��id

    protocol_header(ui8  proto_id) {
        check_sum = 0;
        protocol = proto_id;
    }
} protocol_header;

typedef struct protocol_ack : public protocol_header {
    ui16  win_size;    //�������ڴ�С
    ui8   seq_num_ack_count;    //���ȷ�Ϻ�����
    //ui32* seq_num_ack;
    protocol_ack()
        :protocol_header(proto_ack) {
        win_size = 0;
        seq_num_ack_count = 0;
    }
} protocol_ack;

typedef struct protocol_ctrl : public protocol_header {
    ui32 seq_num;     //���
    ui16 cmd;         //����

    protocol_ctrl()
        :protocol_header(proto_ctrl) {
        seq_num = 0;
        cmd = 0;
    }
} protocol_ctrl;

typedef struct protocol_ctrl_ack : public protocol_header {
    ui32 seq_num_ack; //���ȷ�Ϻ�
    ui16 cmd;         //����
    ui16 error;       //����ctrl_ack_error

    typedef enum ctrl_ack_error {
        ctrl_ack_success = 0, //����ɹ�

    } ctrl_ack_error;

    protocol_ctrl_ack()
        :protocol_header(proto_ctrl_ack) {
        seq_num_ack = 0;
        cmd = 0;
        error = ctrl_ack_success;
    }
} protocol_ctrl_ack;

typedef struct protocol_connect : public protocol_header {
    ui32 seq_num;     //���
    ui16 ack_timeout; //ȷ�ϳ�ʱ
    ui16 data_size; //���ݴ�С
    //ui8* data;

    protocol_connect()
        :protocol_header(proto_connect) {
        seq_num = 0;
        ack_timeout = 0;
        data_size = 0;
    }
} protocol_connect;

typedef struct protocol_connect_ack : public protocol_header {
    ui32 seq_num_ack; //���ȷ�Ϻ�
    ui16 win_size;    //�������ڴ�С
    ui16 error;       //connect_ack_error
    ui16 ack_timeout; //ȷ�ϳ�ʱ
    ui16 heart_beat_timeout;     //������ʱ s

    typedef enum connect_ack_error {
        connect_ack_success = 0, //�������ӳɹ�
        connect_ack_deny_income, //������δ���ڼ���״̬
        connect_ack_busy,        //������æ
        connect_ack_toomuch,     //�ﵽ����������
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
    ui32 seq_num;     //���
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
    ui32 seq_num;     //���

    protocol_heartbeat()
        :protocol_header(proto_heartbeat) {
        seq_num = 0;
    }
} protocol_heartbeat;

typedef struct protocol_data : public protocol_header {
    ui32 seq_num;     //���
    ui32 seq_num_ack; //���ȷ�Ϻ�
    ui8  flags;       //��־λ
    ui16 data_size;   //���ݴ�С
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

    ui16 data_size; //���ݴ�С
    //ui8* data;

    protocol_data_noack()
        :protocol_header(proto_data_noack) {

        data_size = 0;
    }
} protocol_data_noack;

typedef struct protocol_udp_data : public protocol_header {
    ui16 data_size; //���ݴ�С
    //ui8* data;

    protocol_udp_data()
        :protocol_header(proto_udp_data) {
        data_size = 0;
    }
} protocol_udp_data;

bool protocol_set_header(protocol_header* header);
bool protocol_check_header(protocol_header* header, ui16 size);
 
#endif