#ifndef THREAD_H
#define THREAD_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"
#include "timer.h"

typedef enum thread_state{
    thread_state_running = 0,
    thread_state_quit = 1,
}thread_state;

typedef void* thread_handle;
typedef void* mutex_handle;
typedef void* mutex_cond_handle;

typedef struct thread_info{
    mutex_handle      lock;
    mutex_cond_handle cond;
    i32               state;
    void*             param;
}thread_info;

typedef void*(__cdecl *thread_proc) (thread_handle handle);
typedef void(*thread_end_callback)(thread_handle handle);

i32 thread_startup();
i32 thread_cleanup();

thread_handle thread_create(thread_proc proc, void* param, thread_end_callback cb);
void thread_destroy(thread_handle handle, i32 exit_code);
thread_info* thread_get_thread_info(thread_handle handle);

mutex_handle mutex_create();
void mutex_destroy(mutex_handle lock);
void mutex_lock(mutex_handle lock);
void mutex_unlock(mutex_handle lock);

mutex_cond_handle mutex_cond_create();
void mutex_cond_destroy(mutex_cond_handle handle);
void mutex_cond_wait(mutex_cond_handle handle, mutex_handle lock, const timer_val* timeout = 0, bool* is_timeout = 0);
void mutex_cond_signal(mutex_cond_handle handle);

#ifdef PLATFORM_CONFIG_TEST
void thread_test();
#endif

#endif