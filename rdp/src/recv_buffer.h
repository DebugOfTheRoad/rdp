#ifndef RECVBUFFER_H
#define RECVBUFFER_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "buffer.h"


//typedef std::map<ui32, recv_buffer_ex*> recv_buffer_list;

typedef struct recv_buffer {
   // recv_buffer_list rbl;           //���ն���
   // ui32             last_ack;      //���һ���Ѿ�ȷ�Ϲ��ı��
    //ui32             last_need_ack; //���ն������ܹ�ȷ�ϵ����һ�����
}recv_buffer;
 
typedef struct recv_buffer_ex : public recv_buffer{

}recv_buffer_ex;


#endif