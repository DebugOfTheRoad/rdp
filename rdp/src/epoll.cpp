#include "epoll.h"
#include "thread.h"
#include "recv.h"
#include "alloc.h"

#ifdef PLATFORM_OS_LINUX

static int s_eid = -1;

static thread_handle s_thread_list[256] = { 0 };
static recv_result_callback s_recv_result_callback = 0;
static recv_result_timeout_callback  s_recv_result_timeout_callback = 0;

i32 epoll_create(recv_result_callback cb, recv_result_timeout_callback tcb)
{
    if (!cb || !tcb) {
        return RDPERROR_INVALIDPARAM;
    }
    s_recv_result_callback = cb;
    s_recv_result_timeout_callback = tcb;

    const rdp_startup_param& param = socket_get_startup_param();

    s_eid = epoll_create(param.max_sock);
    if (s_eid < 0) {
        return RDPERROR_SYSERROR;
    }
    for (ui16 i = 0; i < param.recv_thread_num; ++i) {
        s_thread_list[i] = thread_create(recv_thread_proc, 0, 0);
        if (!s_thread_list[i]) {
            return RDPERROR_SYSERROR;
        }
    }
    return  RDPERROR_SUCCESS;
}
i32 epoll_destroy()
{
    s_recv_result_callback = 0;
    s_recv_result_timeout_callback = 0;

    for (ui16 i = 0; s_thread_list[i] != 0; ++i) {
        thread_info* ti = thread_get_thread_info(s_thread_list[i]);
        ti->state = thread_state_quit;
    }

    for (ui16 i = 0; s_thread_list[i] != 0; ++i) {
        thread_destroy(s_thread_list[i], -1);
        s_thread_list[i] = 0;
    }

    if (s_eid >= 0) {
        ::close(s_eid);
        s_eid = -1;
    }

    return  RDPERROR_SUCCESS;
}
i32 epoll_attach(SOCKET sock, bool v4)
{
    int ret = RDPERROR_SUCCESS;
    do {
        epoll_event ev;
        memset(&ev, 0, sizeof(epoll_event));
        ev.events = EPOLLIN | EPOLLERR;
        ev.data.fd = sock;
        int n = ::epoll_ctl(s_eid, EPOLL_CTL_ADD, sock, &ev);
        if (n < 0) {
            ret = RDPERROR_SYSERROR;
            break;
        }

    } while (0);

    return ret;
}
i32 epoll_detach(SOCKET sock)
{
    int ret = RDPERROR_SUCCESS;
    do {
        epoll_event ev;
        memset(&ev, 0, sizeof(epoll_event));
        int n = ::epoll_ctl(s_eid, EPOLL_CTL_DEL, sock, &ev);
        if (n < 0) {
            ret = RDPERROR_SYSERROR;
            break;
        }
    } while (0);

    return ret;
}
void* __cdecl recv_thread_proc(thread_handle handle)
{
    thread_info* ti = thread_get_thread_info(handle);
    const rdp_startup_param& param = socket_get_startup_param();
    buffer buf = buffer_create(param.recv_buf_size);
    char addr[sizeof(sockaddr_in6)] = { 0 };
    int addrlen = sizeof(addr);

    timer_val now = { 0 };
    recv_result result = {0};
    epoll_event* ev = new epoll_event[param.max_sock[;

    while (ti->state != thread_state_quit) {
        sigset_t origmask;
        sigprocmask(SIG_SETMASK, &sigmask, &origmask);
        int nfds = epoll_pwait(s_eid, ev, param.max_sock, 1/*EPOLL_TIME_OUT*/);//(linux core :2.6.19) (glibc :2.6)
        sigprocmask(SIG_SETMASK, &origmask, NULL);
        if (ti->state == thread_state_quit) {
            break;
        }
        if (!s_recv_result_callback || !s_recv_result_timeout_callback){
            break;
        }
        now = timer_get_current_time();
        if (nfds == 0) {//timeout
            s_recv_result_timeout_callback(handle, now);
        } else {
            for (int i = 0; i < nfds; ++i) {
                if (ev[i].events & EPOLLIN)) {
                    SOCKET sock = ev[i].data.fd;
                    addrlen = sizeof(addr);
                    int ret = socket_api_recvfrom(sock, buf, 0, (sockaddr*)addr, &addrlen);
                    if (ret >= 0)  {
                        result.sock = sock;
                        result.addr = (sockaddr*)addr;
                        result.buf = buf;
                        result.now = now;
                        s_recv_result_callback(handle, &result);
                    }
                    epoll_event ev;
                    memset(&ev, 0, sizeof(epoll_event));
                    epoll_ctl(s_eid, EPOLL_CTL_MOD, sock, &ev);
                }
            }
        }

    }

    delete []ev;
    return 0;
}

#endif
