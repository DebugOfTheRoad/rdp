#include "alloc.h"
#include <malloc.h>

void* alloc_new(ui32 size)
{
    return malloc(size);
}
void  alloc_delete(void*p)
{
    if (!p) {
        return;
    }
    free(p);
}