#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_win.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN

#include <coldforce/net/co_tcp_win.h>

#pragma comment(lib, "ws2_32.lib")

//---------------------------------------------------------------------------//
// network (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_win_net_setup(
    void
)
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }

    if (!co_win_tcp_load_functions())
    {
        return false;
    }

    return true;
}

void
co_win_net_cleanup(
    void
)
{
    WSACleanup();
}

bool
co_win_net_extension_setup(
    co_socket_t* sock,
    size_t receive_buffer_size,
    co_win_net_extension_t* win
)
{
    win->io_ctxs = NULL;

    win->receive.io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (win->receive.io_ctx == NULL)
    {
        return false;
    }

    if (sock->type == CO_SOCKET_TYPE_UDP)
    {
        win->receive.io_ctx->id = CO_WIN_NET_IO_ID_UDP_RECEIVE;
    }
    else
    {
        win->receive.io_ctx->id = CO_WIN_NET_IO_ID_TCP_RECEIVE;
    }

    win->receive.io_ctx->sock = sock;

    win->receive.size = 0;
    win->receive.index = 0;
    win->receive.new_size = 0;
    win->receive.remote_net_addr = NULL;

    win->receive.buffer.len = (ULONG)receive_buffer_size;
    win->receive.buffer.buf =
        co_mem_alloc(win->receive.buffer.len);

    if (win->receive.buffer.buf == NULL)
    {
        co_mem_free(win->receive.io_ctx);
        win->receive.io_ctx = NULL;

        return false;
    }

    return true;
}

void
co_win_net_extension_cleanup(
    co_win_net_extension_t* win
)
{
    if (win->receive.io_ctx != NULL)
    {
        co_win_destroy_io_ctx(win->receive.io_ctx);
        win->receive.io_ctx = NULL;
    }

    if (win->receive.buffer.buf != NULL)
    {
        co_mem_free_later(win->receive.buffer.buf);
        win->receive.buffer.buf = NULL;
    }

    if (win->receive.remote_net_addr != NULL)
    {
        co_mem_free(win->receive.remote_net_addr);
        win->receive.remote_net_addr = NULL;
    }

    if (win->io_ctxs != NULL)
    {
        co_list_destroy(win->io_ctxs);
        win->io_ctxs = NULL;
    }
}

bool
co_win_net_send(
    co_socket_t* sock,
    co_win_net_extension_t* win,
    const void* data,
    size_t data_size
)
{
    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASend(
        sock->handle, &buf, 1,
        &sent_size, 0,
        NULL, NULL);

    return (result == 0);
}

ssize_t
co_win_net_send_async(
    co_socket_t* sock,
    co_win_net_extension_t* win,
    const void* data,
    size_t data_size
)
{
    if (!sock->remote.is_open)
    {
        return -1;
    }

    co_win_net_io_ctx_t* io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (io_ctx == NULL)
    {
        return -1;
    }

    memset(&io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    if (sock->type == CO_SOCKET_TYPE_UDP)
    {
        io_ctx->id = CO_WIN_NET_IO_ID_UDP_SEND_ASYNC;
    }
    else
    {
        io_ctx->id = CO_WIN_NET_IO_ID_TCP_SEND_ASYNC;
    }

    io_ctx->sock = sock;

    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASend(
        sock->handle, &buf, 1,
        &sent_size, 0,
        (LPWSAOVERLAPPED)io_ctx, NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            return -1;
        }

        if (win->io_ctxs == NULL)
        {
            co_list_ctx_st list_ctx = { 0 };
            list_ctx.destroy_value = (co_item_destroy_fn)co_win_destroy_io_ctx;
            win->io_ctxs = co_list_create(&list_ctx);
        }

        co_list_add_tail(win->io_ctxs, io_ctx);

        return 0;
    }
    else
    {
        co_mem_free(io_ctx);

        co_event_id_t event_id;

        if (sock->type == CO_SOCKET_TYPE_UDP)
        {
            event_id = CO_NET_EVENT_ID_UDP_SEND_ASYNC_COMPLETE;
        }
        else
        {
            event_id = CO_NET_EVENT_ID_TCP_SEND_ASYNC_COMPLETE;
        }

        co_thread_send_event(
            sock->owner_thread,
            event_id,
            (uintptr_t)sock,
            (uintptr_t)data_size);

        return (ssize_t)data_size;
    }
}

bool
co_win_net_receive_start(
    co_socket_t* sock,
    co_win_net_extension_t* win
)
{
    memset(&win->receive.io_ctx->ol,
        0x00, sizeof(WSAOVERLAPPED));

    win->receive.size = 0;
    win->receive.index = 0;

    if (win->receive.new_size > 0)
    {
        void* new_buffer =
            co_mem_alloc(win->receive.new_size);

        if (new_buffer != NULL)
        {
            co_mem_free(win->receive.buffer.buf);
            win->receive.buffer.buf = new_buffer;

            win->receive.buffer.len =
                (ULONG)win->receive.new_size;
        }

        win->receive.new_size = 0;
    }

    DWORD flags = 0;
    DWORD data_size = 0;

    int result = WSARecv(
        sock->handle,
        &win->receive.buffer, 1,
        &data_size,
        &flags,
        (LPWSAOVERLAPPED)win->receive.io_ctx,
        NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            if (sock->type != CO_SOCKET_TYPE_UDP)
            {
                co_thread_send_event(
                    sock->owner_thread,
                    CO_NET_EVENT_ID_TCP_CLOSE,
                    (uintptr_t)sock,
                    0);
            }

            return false;
        }
    }
    else
    {
        co_event_id_t event_id;

        if (sock->type == CO_SOCKET_TYPE_UDP)
        {
            event_id = CO_NET_EVENT_ID_UDP_RECEIVE_READY;
        }
        else
        {
            event_id = CO_NET_EVENT_ID_TCP_RECEIVE_READY;
        }

        co_thread_send_event(
            sock->owner_thread,
            event_id,
            (uintptr_t)sock,
            (uintptr_t)data_size);
    }

    return true;
}

ssize_t
co_win_net_receive(
    co_socket_t* sock,
    co_win_net_extension_t* win,
    void* buffer,
    size_t buffer_size
)
{
    if (win->receive.size == 0)
    {
        return co_socket_handle_receive(
            sock->handle, buffer, buffer_size, 0);
    }

    size_t data_size =
        co_min(win->receive.size, buffer_size);

    memcpy(buffer,
        &win->receive.buffer.buf[win->receive.index],
        data_size);

    win->receive.index += data_size;
    win->receive.size -= data_size;

    return (ssize_t)data_size;
}

#endif // CO_OS_WIN
