#ifndef BUFFER_H
#define BUFFER_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include <memory.h>

struct buffer{
    ui32 length;
    ui32 capcity;
    ui8* ptr;
};
buffer buffer_create(ui32 length);
buffer buffer_create(const ui8* buf, ui32 length);
buffer buffer_create(const buffer& buf);
void buffer_destroy(buffer& buf);
#endif