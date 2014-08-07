#ifndef RECVBUFFER_H
#define RECVBUFFER_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "buffer.h"


typedef struct recv_buffer : public buffer{

}recv_buffer;
 
typedef struct recv_buffer_ex : public recv_buffer{

}recv_buffer_ex;


#endif