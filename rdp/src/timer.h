#ifndef TIMER_H
#define TIMER_H
#include "../include/lint.h"
#include "../include/platform.h"
#include "../include/rdp_def.h"
#include "config.h"

struct timer_val {
    long tv_sec;//seconds √Î
    long tv_msec;//millisecond ∫¡√Î 10-3
    long tv_usec;//microseconds  Œ¢√Î 10-6
};
 
typedef void* timer_handle;

ui64 timer_time_val_to_ui64(const timer_val& time);//microseconds
timer_val timer_ui64_to_time_val(ui64 time);
timer_val timer_get_current_time();
timer_val timer_after_time(const timer_val& time_from, const timer_val& time_interval);
ui64 timer_sub_msec(const timer_val& now, const timer_val& before);
bool timer_is_empty(const timer_val& time);
void timer_empty(timer_val& time);

timer_handle timer_create();
void timer_destroy(timer_handle handle);
void timer_sleep(timer_handle handle, const timer_val& time_interval);
void timer_sleep_to(timer_handle handle, const timer_val& time);
void timer_sleep_wakeup(timer_handle handle);

#ifdef PLATFORM_CONFIG_TEST
void timer_test();
#endif

#endif