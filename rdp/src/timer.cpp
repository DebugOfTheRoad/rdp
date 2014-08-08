#include "timer.h"
#include "thread.h"
#include "alloc.h"
#include "test.h"

#ifdef PLATFORM_OS_WINDOWS
#include <windows.h>
#include <sys/timeb.h>
#elif defined(PLATFORM_OS_LINUX)
#elif defined(PLATFORM_OS_UNIX)
#endif

typedef struct timer {
    mutex_handle      lock;
    mutex_cond_handle cond;
} timer;

ui64 timer_time_val_to_ui64(const timer_val& time)
{
    return time.tv_sec * 1000000 + time.tv_msec * 1000 + time.tv_usec;
}
timer_val timer_ui64_to_time_val(ui64 time)
{
    timer_val t;
    t.tv_sec = (long)(time / 1000000);
    t.tv_msec = (long)(time / 1000);
    t.tv_usec = (long)(time - t.tv_sec - t.tv_msec);
    return t;
}
timer_val timer_get_current_time()
{
    timer_val ret;
#ifndef PLATFORM_OS_WINDOWS
    timeval now;
    gettimeofday(&now, 0);
    ret.tv_sec = now.tv_sec;
    ret.tv_msec = now.tv_usec / 1000;
    ret.tv_usec %= 1000;
#else
    struct timeb  now;
    ftime(&now);
    ret.tv_sec = (long)now.time;
    ret.tv_msec = now.millitm;
    ret.tv_usec = 0;
#endif
    return ret;
}
timer_val timer_after_time(const timer_val& time_from, const timer_val& time_interval)
{
    timer_val t = time_from;
    t.tv_sec += time_interval.tv_sec;
    t.tv_msec += time_interval.tv_msec;
    t.tv_usec += time_interval.tv_usec;

    t.tv_msec += t.tv_usec / 1000;
    t.tv_sec += t.tv_msec / 1000;

    t.tv_msec %= 1000;
    t.tv_usec %= 1000;

    return t;
}
ui64 timer_sub_msec(const timer_val& now, const timer_val& before)
{
    return (now.tv_sec - before.tv_sec) * 1000 + now.tv_msec - before.tv_msec;
}
bool timer_is_empty(const timer_val& time)
{
    return (time.tv_sec == 0) && (time.tv_msec == 0) && (time.tv_usec == 0);
}
void timer_empty(timer_val& time)
{
    time.tv_sec = 0;
    time.tv_msec = 0;
    time.tv_usec = 0;
}
timer_handle timer_create()
{
    timer* t = alloc_new_object<timer>();
    t->lock = mutex_create();
    t->cond = mutex_cond_create();
    return (timer_handle)t;
}
void timer_destroy(timer_handle handle)
{
    if (!handle) {
        return;
    }
    timer_sleep_wakeup(handle);
    timer* t = (timer*)handle;
    mutex_destroy(t->lock);
    mutex_cond_destroy(t->cond);
    alloc_delete_object(t);
}
void timer_sleep(timer_handle handle, const timer_val& time_interval)
{
    timer_val now = timer_get_current_time();
    timer_val next = timer_after_time(now, time_interval);
    timer_sleep_to(handle, next);
}
void timer_sleep_to(timer_handle handle, const timer_val& time)
{
    if (!handle) {
        return;
    }
    timer* t = (timer*)handle;
    mutex_cond_wait(t->cond, t->lock, &time);
}
void timer_sleep_wakeup(timer_handle handle)
{
    if (!handle) {
        return;
    }
    timer* t = (timer*)handle;
    mutex_cond_signal(t->cond);
}
#ifdef PLATFORM_CONFIG_TEST
void timer_test()
{
    //test
    timer_val v1 = timer_get_current_time();
    timer_val v0;
    v0.tv_sec = 2;
    v0.tv_msec = 151;
    v0.tv_usec = 140;
    timer_handle h = timer_create();
    timer_sleep(h, v0);
    timer_destroy(h);
    timer_val v2 = timer_get_current_time();
}
#endif