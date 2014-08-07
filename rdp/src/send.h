#ifndef SEND_H
#define SEND_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "send_buffer.h"
 
i32 send_send(send_buffer_ex* send_buf);

#endif