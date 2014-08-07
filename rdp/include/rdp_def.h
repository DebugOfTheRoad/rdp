#ifndef RDPDEF_H
#define RDPDEF_H

#include "lint.h"

#define RDP_SDK_VERSION 0x00010001
#define RDP_VERSION "0.1.0.1"

//���� (income)  :�����ⲿ��������(��������ɫ)
//���� (outcome) :���������ⲿ(�ͻ��˽�ɫ)
//rdp socket ����ͬʱ֧��income �� outcome,��ͬʱ���������Ϳͻ���

//->rdp_startup
//->rdp_socket_create
//->rdp_socket_bind
//->[rdp_socket_listen:������ܴ���,��Ҫ���ô˷���]
//   |->�Ự:rdp_socket_connect;rdp_session_send;rdp_session_close;
//   |->�ǻỰrdp_udp_send;
//->rdp_socket_close
//->rdp_cleanup

//rdp socket״̬
typedef enum RDPSOCKETSTATUS {
    RDPSOCKETSTATUS_NONE = 0, // ��
    RDPSOCKETSTATUS_INIT,     // ��ʼ
    RDPSOCKETSTATUS_BINDED,   // �Ѱ�
    RDPSOCKETSTATUS_LISTENING,// ����
    RDPSOCKETSTATUS_CLOSING,  // �ر���
} RDPSOCKETSTATUS;

//�Ự״̬
typedef enum RDPSESSIONSTAUS {
    RDPSESSIONSTATUS_INIT = 0,     // �Ự��ʼ״̬
    RDPSESSIONSTATUS_CONNECTING,   // �Ự������
    RDPSESSIONSTATUS_CONNECTED,    // �Ự������
    RDPSESSIONSTATUS_BROKEN,       // ������ʱ��ᵼ�½�״̬ת����RDPSESSIONSTATUS_BROKEN���ȴ�һ��ʱ�����Ȼ��ͨ�ţ��رջỰ
    RDPSESSIONSTATUS_DISCONNECTING,// ���ڶ˿��Ự
} RDPOUTCOMESESSIONSTAUS;

typedef enum RDPERROR {
    RDPERROR_SUCCESS = 0,             //�޴���

    RDPERROR_UNKNOWN = -1,            //δ֪����
    RDPERROR_NOTINIT = -2,            //δ��ʼ�����߳�ʼ��ʧ��
    RDPERROR_INVALIDPARAM =-100,      //��Ч�Ĳ���(��ָ���)
    RDPERROR_SYSERROR,                //ϵͳapi����,��rdp_getsyserror��ȡ������

    RDPERROR_SOCKET_RUNOUT ,           //socket������
    RDPERROR_SOCKET_INVALIDSOCKET ,    //��Ч��socket
    RDPERROR_SOCKET_BADSTATE ,         //�����socket״̬

    RDPERROR_SOCKET_ONCONNECTNOTSET ,   //on_connectδ����
    RDPERROR_SOCKET_ONACCEPTNOTSET ,    //on_acceptδ����
    RDPERROR_SOCKET_ONDISCONNECTNOTSET ,//on_disconnectδ����
    RDPERROR_SOCKET_ONRECVNOTSET,       //on_recvδ����
    RDPERROR_SOCKET_ONUDPRECVNOTSET ,   //on_udp_recvδ����

    RDPERROR_SESSION_INVALIDSESSIONID , //��Ч��sessionid
    RDPERROR_SESSION_BADSTATE ,         //����Ļػ�״̬
} RDPERROR;

typedef ui64 RDPSOCKET;     // != 0
typedef ui64 RDPSESSIONID;  // != 0
#define RDPMAXSOCKET 256    //rdp ֧�ִ��������rdp socket����


struct sockaddr;
typedef struct rdp_on_connect_param{
    i32          err;
    RDPSOCKET    sock;
    RDPSESSIONID session_id;
}rdp_on_connect_param;

typedef struct rdp_on_before_accept{
    RDPSOCKET        sock;
    RDPSESSIONID     session_id;
    const sockaddr*  addr;
    ui32             addrlen;
    const ui8*       buf;
    ui32             buf_len;
}rdp_on_before_accept;

typedef rdp_on_before_accept rdp_on_accept;

typedef struct rdp_on_disconnect_param{
    i32          err;
    ui16         reason;
    RDPSOCKET    sock;
    RDPSESSIONID session_id;

    typedef enum disconnect_reason{
        disconnect_reason_none = 0,
    }disconnect_reason;
}rdp_on_disconnect_param;

typedef struct rdp_on_recv_param{
    RDPSOCKET        sock;
    RDPSESSIONID     session_id;
    const ui8*       buf;
    ui32             buf_len;
}rdp_on_recv_param;

typedef struct rdp_on_send_param{
    i32              err;
    RDPSOCKET        sock;
    RDPSESSIONID     session_id;
    ui32             local_send_queue_size;
    ui32             peer_unused_recv_queue_size;
}rdp_on_send_param;

typedef struct rdp_on_udp_recv_param{
    RDPSOCKET        sock;
    const sockaddr*  addr;
    ui32             addrlen;
    const ui8*       buf;
    ui32             buf_len;
}rdp_on_udp_recv_param;

typedef struct rdp_on_udp_send_param{
    i32              err;
    RDPSOCKET        sock;
    RDPSESSIONID     session_id;
    const sockaddr*  addr;
    ui32             addrlen;
}rdp_on_udp_send_param;

typedef struct rdp_startup_param {
    ui32 version; // rdp sdk �汾�� RDP_SDK_VERSION
    ui16 max_sock;// ���rdp socket����(Ӧ��С�ڵ���RDPMAXSOCKET)
    ui16 recv_thread_num; // ���ݽ����߳�����:��̨���ݽ����߳�����
    ui32 recv_buf_size;   // ���ݽ��ջ�������С:���ݸ�recvfrom�Ļ�������С

    //on_connect �������ӻص�,��������ô˻ص�,����������
    void(__cdecl*on_connect)(const rdp_on_connect_param& param);
    //on_before_accept ���ܴ�������ǰ,����ô˻ص�,����������������,����Ϊ��;����false���ܾ�����������(������Ӧ�����)
    bool(__cdecl*on_before_accept)(const rdp_on_before_accept& param);
    //on_accept �������ӻص�,��������ô˻ص�,����������
    void(__cdecl*on_accept)(const rdp_on_accept& param);
    //on_disconnect ���ӶϿ��ص�,��������
    void(__cdecl*on_disconnect)(const rdp_on_disconnect_param& param);
    //on_recv ���ݽ��ջص�,��������
    void(__cdecl*on_recv)(const rdp_on_recv_param& param);
    //on_send ����,���ɿ������ݽ��ջص�,���������ݵķ���ʹ��rdp_session_send
    void(__cdecl*on_send)(const rdp_on_send_param& param);
    //on_udp_recv ������,���ɿ������ݽ��ջص�,���������ݵķ���ʹ��rdp_udp_send
    void(__cdecl*on_udp_recv)(const rdp_on_udp_recv_param& param);
    //on_udp_send ������,���ɿ������ݽ��ջص�,���������ݵķ���ʹ��rdp_udp_send
    void(__cdecl*on_udp_send)(const rdp_on_udp_send_param& param);
} rdp_startup_param;

typedef struct rdp_socket_create_param {
    bool v4;                    // �Ƿ���ipv4
    ui16 heart_beat_timeout;    // ������ʱ
    ui32 max_recv_queue_size;   // ���ն�������С
    ui32 in_session_hash_size;  // ����Ựhash��С
    ui32 in_session_hash_lock_sessions;  // ����ػ�����������(���ٸ��Ự����һ����)
    ui32 out_session_hash_size;          // �����ػ�hash��С
    ui32 out_session_hash_lock_sessions; // �����ػ�����������(���ٸ��Ự����һ����)
} rdp_socket_create_param;

#endif