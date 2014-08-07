#include "thread.h"
#include "test.h"
#include <set>
#include <map>

//#define USE_PTHREAD

#ifndef PLATFORM_OS_WINDOWS
#undef  USE_PTHREAD
#define USE_PTHREAD
#endif

#ifdef USE_PTHREAD
#include <pthread.h>
#else
#include <windows.h>
#endif

#ifdef USE_PTHREAD
typedef pthread_t       THREAD_HANDLE;
typedef pthread_mutex_t MUTEX_HANDLE;
typedef pthread_cond_t  COND_HANDLE;
#define MUTEX_INVALID_HANDLE PTHREAD_MUTEX_INITIALIZER
#else
typedef HANDLE           THREAD_HANDLE;
typedef CRITICAL_SECTION MUTEX_HANDLE;
typedef HANDLE           COND_HANDLE;
#define MUTEX_INVALID_HANDLE CRITICAL_SECTION()
#endif

#ifdef USE_PTHREAD
#ifdef _MSC_VER
#pragma comment(lib, "pthreadVC2.lib")
#endif
#endif

typedef struct thread_info_ex : public thread_info {
#ifdef USE_PTHREAD
    THREAD_HANDLE* handle;
#else
    THREAD_HANDLE handle;
#endif
    thread_proc   proc;
    thread_end_callback cb;
} thread_info_ex;

#ifdef PLATFORM_CONFIG_DEBUG
typedef std::set<MUTEX_HANDLE*> mutex_list;

#ifdef USE_PTHREAD
typedef std::set<COND_HANDLE*> cond_list;
#else
typedef std::set<COND_HANDLE> cond_list;
#endif

static mutex_list   s_mutex_list;
static MUTEX_HANDLE s_mutex = MUTEX_INVALID_HANDLE;

static cond_list    s_cond_list;
static MUTEX_HANDLE s_cond_list_mutex = MUTEX_INVALID_HANDLE;
#endif


static thread_handle thread_info_ex2thread_handle(thread_info_ex* ti)
{
    return (thread_handle)ti;
}
static thread_info_ex* thread_handle2thread_info_ex(thread_handle handle)
{
    return (thread_info_ex*)handle;
}

i32 thread_startup()
{
#ifdef PLATFORM_CONFIG_DEBUG
#ifdef USE_PTHREAD
    pthread_mutex_init(&s_mutex, 0);
    pthread_mutex_init(&s_cond_list_mutex, 0);
#else
    ::InitializeCriticalSection(&s_mutex);
    ::InitializeCriticalSection(&s_cond_list_mutex);
#endif
#endif
    return RDPERROR_SUCCESS;
}
i32 thread_cleanup()
{
#ifdef PLATFORM_CONFIG_DEBUG
#ifdef USE_PTHREAD
    pthread_mutex_destroy(&s_mutex);
    pthread_mutex_destroy(&s_cond_list_mutex);
#else
    DeleteCriticalSection(&s_mutex);
    DeleteCriticalSection(&s_cond_list_mutex);
#endif
    s_mutex = MUTEX_INVALID_HANDLE;
    s_cond_list_mutex = MUTEX_INVALID_HANDLE;
#endif
    return RDPERROR_SUCCESS;
}
#ifndef USE_PTHREAD
DWORD WINAPI thread_no_pthread_proc(LPVOID lpThreadParameter)
{
    thread_info_ex* ti = (thread_info_ex*)lpThreadParameter;
    return (DWORD)ti->proc((thread_info*)ti);
}
#endif
thread_handle thread_create(thread_proc proc, void* param, thread_end_callback cb)
{
    thread_info_ex* ti = new thread_info_ex;
    ti->lock = mutex_create();
    ti->cond = mutex_cond_create();
    ti->state = thread_state_running;
    ti->param = param;
    ti->proc = proc;
    ti->cb = cb;

#ifdef USE_PTHREAD
    ti->handle = new THREAD_HANDLE;
    if (0 != pthread_create(ti->handle, 0, proc, ti)) {
        delete ti->handle;
        ti->handle = 0;
    }
#else
    ti->handle = (THREAD_HANDLE)CreateThread(NULL, 0, &thread_no_pthread_proc, (void*)ti, 0, 0);
#endif
    if (!ti->handle) {
        delete ti;
        ti = 0;
    }

    return thread_info_ex2thread_handle(ti);
}
void thread_destroy(thread_handle handle, i32 exit_code)
{
    if (!handle) {
        return;
    }
    thread_info_ex* ti = thread_handle2thread_info_ex(handle);
    ti->state = thread_state_quit;
    mutex_cond_signal(ti->cond);

#ifdef USE_PTHREAD
    pthread_cancel(*ti->handle);
    pthread_join(*ti->handle, 0);
#else
    if (WAIT_TIMEOUT == ::WaitForSingleObject(ti->handle, 3000)) {
        TerminateThread(ti->handle, -1);
    }
#endif

    if (ti->cb) {
        ti->cb(handle);
    }
    mutex_cond_destroy(ti->cond);
    mutex_destroy(ti->lock);

#ifdef USE_PTHREAD
    delete ti->handle;
#endif
    delete ti;
}
thread_info* thread_get_thread_info(thread_handle handle)
{
    return (thread_info*)thread_handle2thread_info_ex(handle);
}
mutex_handle mutex_create()
{
    MUTEX_HANDLE* mutex = new MUTEX_HANDLE;
#ifdef USE_PTHREAD
    pthread_mutex_init(mutex, 0);
#else
    ::InitializeCriticalSection(mutex);
#endif
#ifdef PLATFORM_CONFIG_DEBUG
    mutex_lock(&s_mutex);
    s_mutex_list.insert(mutex);
    mutex_unlock(&s_mutex);
#endif
    return (mutex_handle)mutex;
}
void mutex_destroy(mutex_handle lock)
{
    if (!lock) {
        return;
    }
    MUTEX_HANDLE* mutex = (MUTEX_HANDLE*)lock;
#ifdef PLATFORM_CONFIG_DEBUG
    mutex_lock(&s_mutex);
    mutex_list::iterator it = s_mutex_list.find(mutex);
    if (it != s_mutex_list.end()) {
        s_mutex_list.erase(it);
    }
    mutex_unlock(&s_mutex);
#endif

#ifdef USE_PTHREAD
    pthread_mutex_destroy(mutex);
#else
    ::DeleteCriticalSection(mutex);
#endif
    delete mutex;
}
void mutex_lock(mutex_handle lock)
{
    if (!lock) {
        return;
    }
    MUTEX_HANDLE* mutex = (MUTEX_HANDLE*)lock;
#ifdef USE_PTHREAD
    pthread_mutex_lock(mutex);
#else
    ::EnterCriticalSection(mutex);
#endif
}
void mutex_unlock(mutex_handle lock)
{
    if (!lock) {
        return;
    }
    MUTEX_HANDLE* mutex = (MUTEX_HANDLE*)lock;
#ifdef USE_PTHREAD
    pthread_mutex_unlock(mutex);
#else
    ::LeaveCriticalSection(mutex);
#endif
}
mutex_cond_handle mutex_cond_create()
{
#ifdef USE_PTHREAD
    COND_HANDLE* handle = new COND_HANDLE;
    pthread_cond_init(handle, 0);
#ifdef PLATFORM_CONFIG_DEBUG
    pthread_mutex_lock(&s_cond_list_mutex);
    s_cond_list.insert(handle);
    pthread_mutex_unlock(&s_cond_list_mutex);
#endif
#else
    COND_HANDLE handle = CreateEventA(NULL, false, false, NULL);
#ifdef PLATFORM_CONFIG_DEBUG
    EnterCriticalSection(&s_cond_list_mutex);
    s_cond_list.insert(handle);
    LeaveCriticalSection(&s_cond_list_mutex);
#endif
#endif
    return handle;
}
void mutex_cond_destroy(mutex_cond_handle handle)
{
    if (!handle) {
        return;
    }
#ifdef USE_PTHREAD
    COND_HANDLE* cond = (COND_HANDLE*)handle;
#else
    COND_HANDLE cond = (COND_HANDLE)handle;
#endif

#ifdef PLATFORM_CONFIG_DEBUG
    mutex_lock(&s_cond_list_mutex);
    cond_list::iterator it = s_cond_list.find(cond);
    if (it != s_cond_list.end()) {
        s_cond_list.erase(it);
    }
    mutex_unlock(&s_cond_list_mutex);
#endif

#ifdef USE_PTHREAD
    pthread_cond_destroy(cond);
    delete cond;
#else
    CloseHandle(cond);
#endif
}
void mutex_cond_wait(mutex_cond_handle handle, mutex_handle lock, const timer_val* timeout, bool* is_timeout)
{
    if (!handle || !lock) {
        return ;
    }

    MUTEX_HANDLE* mutex = (MUTEX_HANDLE*)lock;
    if (is_timeout) {
        *is_timeout = false;
    }
#ifdef USE_PTHREAD
    mutex_lock(lock);
    COND_HANDLE* cond = (COND_HANDLE*)handle;
    if (!timeout) {
        pthread_cond_wait(cond, mutex);
    } else {
        timespec ts = { timeout->tv_sec, timeout->tv_msec * 1000000 + timeout->tv_usec*1000 };
        int ret = pthread_cond_timedwait(cond, mutex, &ts);
        if (ETIMEDOUT == ret && is_timeout) {
            *is_timeout = true;
        }
    }
    mutex_unlock(lock);
#else
    COND_HANDLE cond = (COND_HANDLE)handle;
    DWORD dwTimeout = -1;
    if (timeout) {
        dwTimeout = timeout->tv_sec*1000 + timeout->tv_msec;
    }
    DWORD ret = ::WaitForSingleObject(cond, dwTimeout);
    if (WAIT_TIMEOUT == ret  && is_timeout) {
        *is_timeout = true;
    }
#endif
}
void mutex_cond_signal(mutex_cond_handle handle)
{
    if (!handle) {
        return;
    }
#ifdef USE_PTHREAD
    COND_HANDLE* cond = (COND_HANDLE*)handle;
    pthread_cond_signal(cond);
#else
    COND_HANDLE cond = (COND_HANDLE)handle;
    SetEvent(cond);
#endif
}

#ifdef PLATFORM_CONFIG_TEST
void thread_test()
{

}
#endif