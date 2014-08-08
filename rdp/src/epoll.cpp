#include "epoll.h"
#include "thread.h"
#include "recv.h"
#include "alloc.h"

#ifdef PLATFORM_OS_LINUX


static int s_eid = -1;

i32 epoll_create(recv_result_callback cb)
{
    int ret = RDPERROR_SUCCESS;
    do {
        s_eid = epoll_create(s_socket_startup_param.max_sock);
        if (s_eid < 0) {
            ret = RDPERROR_SYSERROR;
            break;
        }

    } while (0);

    return  ret;
}
i32 epoll_destroy()
{
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
    buffer buf = buffer_create(s_socket_startup_param.recv_buf_size);
    char addr[sizeof(sockaddr_in6)] = { 0 };
    int addrlen = sizeof(addr);

    recv_result result = {0};
    epoll_event* ev = new epoll_event[s_socket_startup_param.max_sock[;

    while (ti->state != thread_state_quit) {
        int nfds = epoll_wait(s_eid, ev, s_socket_startup_param.max_sock, EPOLL_TIME_OUT);

        for (int i = 0; i < nfds; ++i) {
            if (ev[i].events & EPOLLIN)) {
                SOCKET sock = ev[i].data.fd;
                addrlen = sizeof(addr);
                int ret = socket_api_recvfrom(sock, buf, 0, (sockaddr*)addr, &addrlen);
                if (ret >= 0)  {
                    result.sock = sock;
                    result.addr = (sockaddr*)addr;
                    result.buf = buf;
                    s_recv_result_callback(handle, &result);
                }
                epoll_event ev;
                memset(&ev, 0, sizeof(epoll_event));
                epoll_ctl(s_eid, EPOLL_CTL_MOD, sock, &ev);
            }
        }
    }

    delete []ev;
    return 0;
}

#endif
