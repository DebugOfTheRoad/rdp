
#include "send.h"
#include "socket_api.h"

i32 send_send(send_buffer_ex* send_buf)
{
    i32 ret = socket_api_sendto(send_buf->sock,
                                send_buf->buf,
                                send_buf->flags,
                                send_buf->addr,
                                socket_api_addr_len(send_buf->addr));

    if (ret < 0){
        return RDPERROR_SYSERROR;
    }
    return RDPERROR_SUCCESS;
}