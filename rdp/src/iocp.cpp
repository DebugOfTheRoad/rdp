#include "iocp.h"
#include "thread.h"
#include "socket_api.h"
#include "recv.h"
#include "alloc.h"
#include "socket.h"
#include <map>

#ifdef PLATFORM_OS_WINDOWS

typedef enum iocp_opertion {
    iocp_opertion_recv = 0,
    iocp_opertion_send,
} iocp_opertion;

typedef struct iocp_buffer {
    WSAOVERLAPPED ol;
    ui8           slot;
    SOCKET        socket;
    bool          v4;
    sockaddr*     addr;
    WSABUF        buf;
    iocp_opertion op;
} iocp_buffer;


static HANDLE s_iocp = 0;

static thread_handle s_thread_list[256] = { 0 };
static recv_result_callback s_recv_result_callback = 0;
static recv_result_timeout_callback  s_recv_result_timeout_callback = 0;


void* __cdecl recv_thread_proc(thread_handle handle);

static iocp_buffer* iocp_create_buffer(ui8 slot, SOCKET sock, bool v4)
{
    const rdp_startup_param& param = socket_get_startup_param();

    iocp_buffer* ioBuffer = alloc_new_object<iocp_buffer>();
    SecureZeroMemory((PVOID)&ioBuffer->ol, sizeof(WSAOVERLAPPED));
    ioBuffer->slot = slot;
    ioBuffer->socket = sock;
    ioBuffer->v4 = v4;
    ioBuffer->addr = socket_api_addr_create(v4, 0);
    ioBuffer->op = iocp_opertion_recv;
    ioBuffer->buf.len = param.recv_buf_size;
    ioBuffer->buf.buf = (char*)alloc_new(ioBuffer->buf.len) ;
    memset(ioBuffer->buf.buf, 0, ioBuffer->buf.len);
    return ioBuffer;
}
static void iocp_destroy_buffer(iocp_buffer* ioBuffer)
{
    if (!ioBuffer) {
        return;
    }
    socket_api_addr_destroy(ioBuffer->addr);
    alloc_delete(ioBuffer->buf.buf);
    alloc_delete_object(ioBuffer);
}
i32 iocp_create(recv_result_callback cb, recv_result_timeout_callback tcb)
{
    if (!cb || !tcb) {
        return RDPERROR_INVALIDPARAM;
    }
    s_recv_result_callback = cb;
    s_recv_result_timeout_callback = tcb;
    const rdp_startup_param& param = socket_get_startup_param();
    s_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, param.recv_thread_num);
    if (!s_iocp) {
        return RDPERROR_SYSERROR;
    }

    for (ui16 i = 0; i < param.recv_thread_num; ++i) {
        s_thread_list[i] = thread_create(recv_thread_proc, 0, 0);
        if (!s_thread_list[i]) {
            return RDPERROR_SYSERROR;
        }
    }
    return  RDPERROR_SUCCESS ;
}
i32 iocp_destroy()
{
    s_recv_result_callback = 0;
    s_recv_result_timeout_callback = 0;

    for (ui16 i = 0; s_thread_list[i] != 0; ++i) {
        thread_info* ti = thread_get_thread_info(s_thread_list[i]);
        ti->state = thread_state_quit;

        PostQueuedCompletionStatus(s_iocp, 0, 0, 0);
    }

    for (ui16 i = 0; s_thread_list[i] != 0; ++i) {
        thread_destroy(s_thread_list[i], -1);
        s_thread_list[i] = 0;
    }

    if (s_iocp) {
        CloseHandle(s_iocp);
        s_iocp = 0;
    }

    return  RDPERROR_SUCCESS;
}
i32 iocp_attach(ui8 slot, SOCKET sock, bool v4)
{
    i32 ret = RDPERROR_SUCCESS;
    iocp_buffer* ioBuffer = iocp_create_buffer(slot, sock, v4);
    const rdp_startup_param& param = socket_get_startup_param();

    do {
        HANDLE h = CreateIoCompletionPort((HANDLE)sock, s_iocp,
                                          (ULONG_PTR)ioBuffer->addr,
                                          param.recv_thread_num);
        if (!h) {
            ret = RDPERROR_SYSERROR;
            break;
        }

        int addrlen = ioBuffer->v4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

        DWORD dwFlags = 0, dwRecvs = 0;
        int nRet = WSARecvFrom(ioBuffer->socket,
                               &ioBuffer->buf, 1,
                               &dwRecvs, &dwFlags,
                               (sockaddr *)ioBuffer->addr,
                               &addrlen,
                               (LPOVERLAPPED)ioBuffer, NULL);

        if (nRet == SOCKET_ERROR)  {
            DWORD dwLastError = GetLastError();
            if (dwLastError != WSA_IO_PENDING) {
                ret = RDPERROR_SYSERROR;
                break;
            }
        }
    } while (0);

    if (ret != RDPERROR_SUCCESS) {
        iocp_destroy_buffer(ioBuffer);
    }

    return ret ;
}
i32 iocp_detach(SOCKET sock)
{
    CancelIo((HANDLE)sock);
    return RDPERROR_SUCCESS;
}
void* __cdecl recv_thread_proc(thread_handle handle)
{
    thread_info* ti = thread_get_thread_info(handle);

    DWORD dwTransferd = 0;
    PULONG_PTR   CompletionKey   = NULL;
    LPOVERLAPPED lpOverlapped    = NULL;
    DWORD dwLastError = 0;

    sockaddr_in6 addr = { 0 };
    timer_val now = {0};
    recv_result result = { 0 };
    
    while (ti->state != thread_state_quit) {
        BOOL bRet = GetQueuedCompletionStatus(s_iocp,
                                              &dwTransferd,
                                              (PULONG_PTR)&CompletionKey,
                                              &lpOverlapped, 1/*INFINITE*/);
        if (ti->state == thread_state_quit) {
            break;
        }
        if (!s_recv_result_callback || !s_recv_result_timeout_callback){
            break;
        }
        iocp_buffer* ioBuffer = CONTAINING_RECORD(lpOverlapped, iocp_buffer, ol);
        now = timer_get_current_time();
        if (!bRet) {
            dwLastError = GetLastError();
            if (WAIT_TIMEOUT == dwLastError) {
                s_recv_result_timeout_callback(handle, now);
            }
            else if (ERROR_OPERATION_ABORTED == dwLastError){
                iocp_destroy_buffer(ioBuffer);
            }
            else if (ERROR_PORT_UNREACHABLE == dwLastError){
                 
            }
            else if (ERROR_MORE_DATA == dwLastError){
            }
            continue;
        }
        if (!ioBuffer) {
            continue;
        }

        if (ioBuffer->op == iocp_opertion_recv) {
            memcpy(&addr, ioBuffer->addr, ioBuffer->addr->sa_family == AF_INET ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));
            result.slot = ioBuffer->slot;
            result.sock = ioBuffer->socket;
            result.addr = ioBuffer->addr;
            result.buf.ptr = (ui8*)ioBuffer->buf.buf;
            result.buf.capcity = dwTransferd;
            result.buf.length = result.buf.capcity;
            result.now = now;
            s_recv_result_callback(handle, &result);

            int addrlen = ioBuffer->v4 ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

            DWORD dwFlags = 0, dwRecvs = 0;
            int ret = WSARecvFrom(ioBuffer->socket,
                &ioBuffer->buf, 1,
                &dwRecvs, &dwFlags,
                ioBuffer->addr,
                &addrlen,
                (LPOVERLAPPED)ioBuffer, NULL);

            if (ret == SOCKET_ERROR)  {
                dwLastError = GetLastError();

                if (WSA_IO_PENDING != dwLastError) {
                    if (WSAECONNRESET == dwLastError) {
                        result.sock = ioBuffer->socket;
                        result.addr = (sockaddr*)&addr;
                        result.buf.ptr = (ui8*)ioBuffer->buf.buf;
                        result.buf.capcity = 0;
                        result.buf.length = 0;
                        result.now = now;
                        s_recv_result_callback(handle, &result);
                    }
                    else {
                        break;
                    }
                }
            }
        }
        
    }

    return 0;
}

#endif
