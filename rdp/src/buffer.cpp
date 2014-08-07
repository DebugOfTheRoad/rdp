#include "buffer.h"

buffer buffer_create(ui32 length)
{
    buffer b;
    b.length = length;
    b.capcity = length;
    b.ptr = new ui8[b.capcity];
    return b;
}
buffer buffer_create(const ui8* buf, ui32 length)
{
    buffer b;
    b.length = length;
    b.capcity = length;
    if (b.length > 0) {
        b.ptr = new ui8[b.capcity];
        memcpy(b.ptr, buf, b.length);
    }
    else {
        b.ptr = 0;
    }
    return b;
}
buffer buffer_create(const buffer& buf)
{
    buffer b;
    b.length = buf.length;
    b.capcity = buf.capcity;
    if (b.length > 0) {
        b.ptr = new ui8[b.capcity];
        memcpy(b.ptr, buf.ptr, b.length);
    } else {
        b.ptr = 0;
    }
    return b;
}
void buffer_destroy(buffer& buf)
{
    if (buf.ptr) {
        delete[]buf.ptr;
    }
    buf.ptr = 0;
    buf.length = 0;
    buf.capcity = 0;
}