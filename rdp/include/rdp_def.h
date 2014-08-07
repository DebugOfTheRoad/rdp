#ifndef RDPDEF_H
#define RDPDEF_H

#include "lint.h"

#define RDP_SDK_VERSION 0x00010001
#define RDP_VERSION "0.1.0.1"

//传入 (income)  :接收外部连接请求(服务器角色)
//传出 (outcome) :主动连接外部(客户端角色)
//rdp socket 可以同时支持income 和 outcome,即同时做服务器和客户端

//->rdp_startup
//->rdp_socket_create
//->rdp_socket_bind
//->[rdp_socket_listen:如果接受传入,需要调用此方法]
//   |->会话:rdp_socket_connect;rdp_session_send;rdp_session_close;
//   |->非会话rdp_udp_send;
//->rdp_socket_close
//->rdp_cleanup

//rdp socket状态
typedef enum RDPSOCKETSTATUS {
    RDPSOCKETSTATUS_NONE = 0, // 无
    RDPSOCKETSTATUS_INIT,     // 初始
    RDPSOCKETSTATUS_BINDED,   // 已绑定
    RDPSOCKETSTATUS_LISTENING,// 监听
    RDPSOCKETSTATUS_CLOSING,  // 关闭中
} RDPSOCKETSTATUS;

//会话状态
typedef enum RDPSESSIONSTAUS {
    RDPSESSIONSTATUS_INIT = 0,     // 会话初始状态
    RDPSESSIONSTATUS_CONNECTING,   // 会话连接中
    RDPSESSIONSTATUS_CONNECTED,    // 会话已连接
    RDPSESSIONSTATUS_BROKEN,       // 心跳超时后会导致将状态转换到RDPSESSIONSTATUS_BROKEN，等待一段时间后，仍然无通信，关闭会话
    RDPSESSIONSTATUS_DISCONNECTING,// 正在端开会话
} RDPOUTCOMESESSIONSTAUS;

typedef enum RDPERROR {
    RDPERROR_SUCCESS = 0,             //无错误

    RDPERROR_UNKNOWN = -1,            //未知错误
    RDPERROR_NOTINIT = -2,            //未初始化或者初始化失败
    RDPERROR_INVALIDPARAM =-100,      //无效的参数(空指针等)
    RDPERROR_SYSERROR,                //系统api错误,用rdp_getsyserror获取错误码

    RDPERROR_SOCKET_RUNOUT ,           //socket已用完
    RDPERROR_SOCKET_INVALIDSOCKET ,    //无效的socket
    RDPERROR_SOCKET_BADSTATE ,         //错误的socket状态

    RDPERROR_SOCKET_ONCONNECTNOTSET ,   //on_connect未设置
    RDPERROR_SOCKET_ONACCEPTNOTSET ,    //on_accept未设置
    RDPERROR_SOCKET_ONDISCONNECTNOTSET ,//on_disconnect未设置
    RDPERROR_SOCKET_ONRECVNOTSET,       //on_recv未设置
    RDPERROR_SOCKET_ONUDPRECVNOTSET ,   //on_udp_recv未设置

    RDPERROR_SESSION_INVALIDSESSIONID , //无效的sessionid
    RDPERROR_SESSION_BADSTATE ,         //错误的回话状态
} RDPERROR;

typedef ui64 RDPSOCKET;     // != 0
typedef ui64 RDPSESSIONID;  // != 0
#define RDPMAXSOCKET 256    //rdp 支持创建的最大rdp socket数量


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
    ui32 version; // rdp sdk 版本号 RDP_SDK_VERSION
    ui16 max_sock;// 最大rdp socket数量(应该小于等于RDPMAXSOCKET)
    ui16 recv_thread_num; // 数据接收线程数量:后台数据接收线程数量
    ui32 recv_buf_size;   // 数据接收缓冲区大小:传递给recvfrom的缓冲区大小

    //on_connect 传出连接回调,如果不设置此回调,将不允许传出
    void(__cdecl*on_connect)(const rdp_on_connect_param& param);
    //on_before_accept 接受传入连接前,会调用此回调,可以用来过滤连接,可以为空;返回false将拒绝此连接请求(不会响应请求端)
    bool(__cdecl*on_before_accept)(const rdp_on_before_accept& param);
    //on_accept 传入连接回调,如果不设置此回调,将不允许传入
    void(__cdecl*on_accept)(const rdp_on_accept& param);
    //on_disconnect 连接断开回调,必须设置
    void(__cdecl*on_disconnect)(const rdp_on_disconnect_param& param);
    //on_recv 数据接收回调,必须设置
    void(__cdecl*on_recv)(const rdp_on_recv_param& param);
    //on_send 连接,不可靠的数据接收回调,该类型数据的发送使用rdp_session_send
    void(__cdecl*on_send)(const rdp_on_send_param& param);
    //on_udp_recv 非连接,不可靠的数据接收回调,该类型数据的发送使用rdp_udp_send
    void(__cdecl*on_udp_recv)(const rdp_on_udp_recv_param& param);
    //on_udp_send 非连接,不可靠的数据接收回调,该类型数据的发送使用rdp_udp_send
    void(__cdecl*on_udp_send)(const rdp_on_udp_send_param& param);
} rdp_startup_param;

typedef struct rdp_socket_create_param {
    bool v4;                    // 是否是ipv4
    ui16 heart_beat_timeout;    // 心跳超时
    ui32 max_recv_queue_size;   // 接收队列最大大小
    ui32 in_session_hash_size;  // 传入会话hash大小
    ui32 in_session_hash_lock_sessions;  // 传入回话锁争用力度(多少个会话争用一个锁)
    ui32 out_session_hash_size;          // 传出回话hash大小
    ui32 out_session_hash_lock_sessions; // 传出回话锁争用力度(多少个会话争用一个锁)
} rdp_socket_create_param;

#endif