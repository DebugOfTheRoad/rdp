#ifndef ALLOC_H
#define ALLOC_H

#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include <new.h>

void* alloc_new(ui32 size);
void  alloc_delete(void*p);

template<typename T>
T* alloc_new_object()
{
    void* p = alloc_new(sizeof(T));
    return new(p)T();
}
template<typename T>
void alloc_delete_object(T* p)
{
    if (p) {
        p->~T();
        alloc_delete((void*)p);
    }
}

#endif