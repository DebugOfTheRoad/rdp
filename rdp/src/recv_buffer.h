#ifndef RECVBUFFER_H
#define RECVBUFFER_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "buffer.h"


//typedef std::map<ui32, recv_buffer_ex*> recv_buffer_list;

typedef struct recv_buffer {
   // recv_buffer_list rbl;           //接收队列
   // ui32             last_ack;      //最近一次已经确认过的编号
    //ui32             last_need_ack; //接收队列中能够确认的最后一个编号
}recv_buffer;
 
typedef struct recv_buffer_ex : public recv_buffer{

}recv_buffer_ex;


#endif