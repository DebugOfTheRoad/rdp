#include "iocp.h"
#include "thread.h"
#include "socket_api.h"
#include "recv.h"
#include <map>

#ifdef PLATFORM_OS_WINDOWS

typedef enum iocp_opertion {
    iocp_opertion_recv = 0,
    iocp_opertion_send,
} iocp_opertion;

typedef struct iocp_buffer {
    WSAOVERLAPPED ol;
    SOCKET        socket;
    bool          v4;
    sockaddr*     addr;
    WSABUF        buf;
    iocp_opertion op;
} iocp_buffer;

typedef std::map<HANDLE, iocp_buffer*>iocp_buffer_list;

static HANDLE s_iocp = 0;
static void* s_thread_list[256] = { 0 };
static recv_result_callback s_recv_result_callback = 0;
static iocp_buffer_list s_iocp_buffer_list;
static mutex_handle     s_iocp_buffer_list_lock = 0;

void* __cdecl recv_thread_proc(thread_handle handle);

static iocp_buffer* iocp_create_buffer(SOCKET sock, bool v4)
{
    iocp_buffer* ioBuffer = new iocp_buffer;
    SecureZeroMemory((PVOID)&ioBuffer->ol, sizeof(WSAOVERLAPPED));
    ioBuffer->socket = sock;
    ioBuffer->v4 = v4;
    ioBuffer->addr = socket_api_addr_create(v4, 0);
    ioBuffer->op = iocp_opertion_recv;
    ioBuffer->buf.len = s_socket_startup_param.recv_buf_size;
    ioBuffer->buf.buf = new char[ioBuffer->buf.len];
    memset(ioBuffer->buf.buf, 0, ioBuffer->buf.len);
    return ioBuffer;
}
static void iocp_destroy_buffer(iocp_buffer* ioBuffer)
{
    if (!ioBuffer) {
        return;
    }
    socket_api_addr_destroy(ioBuffer->addr);
    delete []ioBuffer->buf.buf;
    delete ioBuffer;
}
i32 iocp_create(recv_result_callback cb)
{
    if (!cb) {
        return RDPERROR_INVALIDPARAM;
    }
    s_recv_result_callback = cb;
    s_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, s_socket_startup_param.recv_thread_num);
    if (!s_iocp) {
        return RDPERROR_SYSERROR;
    }
    s_iocp_buffer_list_lock = mutex_create();
    for (ui16 i = 0; i < s_socket_startup_param.recv_thread_num; ++i) {
        s_thread_list[i] = thread_create(recv_thread_proc, 0, 0);
        if (!s_thread_list[i]) {
            return RDPERROR_SYSERROR;
        }
    }
    return  RDPERROR_SUCCESS ;
}
i32 iocp_destroy()
{
    for (ui16 i = 0; s_thread_list[i] != 0; ++i) {
        thread_info* ti = thread_get_thread_info(s_thread_list[i]);
        ti->state = thread_state_quit;
    }

    PostQueuedCompletionStatus(s_iocp, 0, NULL, NULL);

    for (ui16 i = 0; s_thread_list[i] != 0; ++i) {
        thread_destroy(s_thread_list[i], -1);
        s_thread_list[i] = 0;
    }

    if (s_iocp) {
        CancelIo(s_iocp);
        CloseHandle(s_iocp);
        s_iocp = 0;
    }
    mutex_destroy(s_iocp_buffer_list_lock);
    s_iocp_buffer_list_lock = 0;

    return  RDPERROR_SUCCESS;
}
i32 iocp_attach(SOCKET sock, bool v4)
{
    i32 ret = RDPERROR_SUCCESS;
    iocp_buffer* ioBuffer = iocp_create_buffer(sock, v4);

    do {
        HANDLE h = CreateIoCompletionPort((HANDLE)sock, s_iocp,
                                          (ULONG_PTR)ioBuffer->addr,
                                          s_socket_startup_param.recv_thread_num);
        if (!h) {
            ret = RDPERROR_SYSERROR;
            break;
        }

        int addrlen = ioBuffer->v4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

        DWORD dwFlags = 0, dwRecvs = 0;
        int ret = WSARecvFrom(ioBuffer->socket,
                              &ioBuffer->buf, 1,
                              &dwRecvs, &dwFlags,
                              (sockaddr *)ioBuffer->addr,
                              &addrlen,
                              (LPOVERLAPPED)ioBuffer, NULL);

        if (ret == SOCKET_ERROR)  {
            DWORD dwLastError = GetLastError();
            if (dwLastError != WSA_IO_PENDING) {
                ret = RDPERROR_SYSERROR;
                break;
            }
        }
        mutex_lock(s_iocp_buffer_list_lock);
        s_iocp_buffer_list[(HANDLE)sock] = ioBuffer;
        mutex_unlock(s_iocp_buffer_list_lock);
    } while (0);

    if (ret != RDPERROR_SUCCESS) {
        iocp_destroy_buffer(ioBuffer);
    }

    return ret ;
}
i32 iocp_detach(SOCKET sock)
{
    mutex_lock(s_iocp_buffer_list_lock);
    iocp_buffer_list::iterator it = s_iocp_buffer_list.find((HANDLE)sock);
    if (it != s_iocp_buffer_list.end()) {
        CancelIo((HANDLE)sock);
        iocp_buffer* ioBuffer = it->second;
        s_iocp_buffer_list.erase(it);
        iocp_destroy_buffer(ioBuffer);
    }
    mutex_unlock(s_iocp_buffer_list_lock);

    return RDPERROR_SUCCESS;
}
void* __cdecl recv_thread_proc(thread_handle handle)
{
    thread_info* ti = thread_get_thread_info(handle);

    DWORD dwTransferd = 0;
    PULONG_PTR   CompletionKey   = NULL;
    LPOVERLAPPED lpOverlapped    = NULL;
    DWORD dwLastError = 0;

    recv_result result = { 0 };

    while (ti->state != thread_state_quit) {
        BOOL bRet = GetQueuedCompletionStatus(s_iocp,
                                              &dwTransferd,
                                              (PULONG_PTR)&CompletionKey,
                                              &lpOverlapped, INFINITE);
        if (ti->state == thread_state_quit) {
            break;
        }
        iocp_buffer* ioBuffer = CONTAINING_RECORD(lpOverlapped, iocp_buffer, ol);
        if (!bRet) {
            continue;
        }
        if (!ioBuffer) {
            continue;
        }

        if (ioBuffer->op != iocp_opertion_recv) {
            continue;
        }

        result.sock = ioBuffer->socket;
        result.addr = ioBuffer->addr;
        result.buf.ptr = (ui8*)ioBuffer->buf.buf;
        result.buf.capcity = dwTransferd;
        result.buf.length = result.buf.capcity;
        s_recv_result_callback(handle, &result);

        int addrlen = ioBuffer->v4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

        DWORD dwFlags = 0, dwRecvs = 0;
        int ret = WSARecvFrom(ioBuffer->socket,
                              &ioBuffer->buf, 1,
                              &dwRecvs, &dwFlags,
                              (sockaddr *)ioBuffer->addr,
                              &addrlen,
                              (LPOVERLAPPED)ioBuffer, NULL);

        if (ret == SOCKET_ERROR)  {
            dwLastError = GetLastError();
            if (dwLastError != WSA_IO_PENDING) {
                break;
            }
        }
    }

    return 0;
}

#endif
