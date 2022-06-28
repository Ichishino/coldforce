#include <coldforce/core/co_std.h>

#include <coldforce/net/co_udp_win.h>
#include <coldforce/net/co_udp.h>
#include <coldforce/net/co_net_worker.h>

#ifdef CO_OS_WIN

//---------------------------------------------------------------------------//
// udp (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

co_socket_handle_t
co_win_udp_socket_create(
    co_address_family_t family
)
{
    SOCKET sock = WSASocketW(
        family, SOCK_DGRAM, IPPROTO_UDP,
        NULL, 0, WSA_FLAG_OVERLAPPED);

    if (sock != CO_SOCKET_INVALID_HANDLE)
    {
        CO_DEBUG_SOCKET_COUNTER_INC();

        BOOL new_value = FALSE;
        DWORD return_bytes = 0;
        
        WSAIoctl(sock,
            _WSAIOW(IOC_VENDOR, 12),
            &new_value, sizeof(new_value),
            NULL, 0, &return_bytes,
            NULL, NULL);
    }

    return sock;
}

bool
co_win_udp_setup(
    co_udp_t* udp,
    size_t receive_buffer_size
)
{
    co_socket_handle_t handle =
        co_win_udp_socket_create(
            udp->sock.local_net_addr.sa.any.ss_family);

    if (handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_item_free_fn)co_win_free_io_ctx;
    udp->win.io_send_ctxs = co_list_create(&list_ctx);

    if (udp->win.io_send_ctxs == NULL)
    {
        co_socket_handle_close(handle);

        return false;
    }

    udp->win.receive.io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (udp->win.receive.io_ctx == NULL)
    {
        co_mem_free(udp->win.io_send_ctxs);
        udp->win.io_send_ctxs = NULL;

        co_socket_handle_close(handle);

        return false;
    }

    udp->win.receive.io_ctx->id = CO_WIN_NET_IO_ID_UDP_RECEIVE;
    udp->win.receive.io_ctx->sock = &udp->sock;

    udp->win.receive.size = 0;
    udp->win.receive.index = 0;
    udp->win.receive.new_size = 0;
    udp->win.receive.buffer.len = (ULONG)receive_buffer_size;
    udp->win.receive.buffer.buf =
        co_mem_alloc(udp->win.receive.buffer.len);

    if (udp->win.receive.buffer.buf == NULL)
    {
        co_mem_free(udp->win.receive.io_ctx);
        udp->win.receive.io_ctx = NULL;

        co_mem_free(udp->win.io_send_ctxs);
        udp->win.io_send_ctxs = NULL;

        co_socket_handle_close(handle);

        return false;
    }

    udp->sock.handle = handle;

    return true;
}

void
co_win_udp_cleanup(
    co_udp_t* udp
)
{
    if (udp->win.receive.io_ctx != NULL)
    {
        co_win_free_io_ctx(udp->win.receive.io_ctx);
        udp->win.receive.io_ctx = NULL;
    }

    if (udp->win.receive.buffer.buf != NULL)
    {
        co_mem_free_later(udp->win.receive.buffer.buf);
        udp->win.receive.buffer.buf = NULL;
    }

    if (udp->win.io_send_ctxs != NULL)
    {
        co_list_destroy(udp->win.io_send_ctxs);
        udp->win.io_send_ctxs = NULL;
    }
}

bool
co_win_udp_send(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
)
{
    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASendTo(
        udp->sock.handle, &buf, 1,
        &sent_size, 0,
        (const struct sockaddr*)remote_net_addr,
        sizeof(co_net_addr_t),
        NULL, NULL);

    return (result == 0);
}

bool
co_win_udp_send_async(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
)
{
    co_win_net_io_ctx_t* io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (io_ctx == NULL)
    {
        return false;
    }

    memset(&io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    io_ctx->id = CO_WIN_NET_IO_ID_UDP_SEND;
    io_ctx->sock = &udp->sock;

    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASendTo(
        udp->sock.handle, &buf, 1,
        &sent_size, 0,
        (const struct sockaddr*)remote_net_addr,
        sizeof(co_net_addr_t),
        (LPWSAOVERLAPPED)io_ctx, NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != ERROR_IO_PENDING)
        {
            return false;
        }
    }

    co_list_add_tail(
        udp->win.io_send_ctxs, (uintptr_t)io_ctx);

    return true;
}

bool
co_win_udp_receive_start(
    co_udp_t* udp
)
{
    memset(&udp->win.receive.io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    udp->win.receive.size = 0;
    udp->win.receive.index = 0;

    if (udp->win.receive.new_size > 0)
    {
        void* new_buffer =
            co_mem_alloc(udp->win.receive.new_size);

        if (new_buffer != NULL)
        {
            co_mem_free(udp->win.receive.buffer.buf);
            udp->win.receive.buffer.buf = new_buffer;

            udp->win.receive.buffer.len =
                (ULONG)udp->win.receive.new_size;
        }

        udp->win.receive.new_size = 0;
    }

    DWORD flags = 0;
    DWORD data_size = 0;
    INT sender_net_addr_size = sizeof(co_net_addr_t);

    int result = WSARecvFrom(
        udp->sock.handle,
        &udp->win.receive.buffer, 1,
        &data_size,
        &flags,
        (struct sockaddr*)&udp->win.receive.remote_net_addr,
        &sender_net_addr_size,
        (LPWSAOVERLAPPED)udp->win.receive.io_ctx,
        NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != ERROR_IO_PENDING)
        {
            return false;
        }
    }

    return true;
}

bool
co_win_udp_receive(
    co_udp_t* udp,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size,
    size_t* data_size
)
{
    if (udp->win.receive.size == 0)
    {
        ssize_t received = co_socket_handle_receive_from(
            udp->sock.handle, remote_net_addr, buffer, buffer_size, 0);

        if (received > 0)
        {
            *data_size = (size_t)received;

            return true;
        }

        return false;
    }

    memcpy(remote_net_addr,
        &udp->win.receive.remote_net_addr, sizeof(co_net_addr_t));

    *data_size = co_min(udp->win.receive.size, buffer_size);

    memcpy(buffer,
        &udp->win.receive.buffer.buf[udp->win.receive.index],
        *data_size);

    udp->win.receive.index += *data_size;
    udp->win.receive.size -= *data_size;

    return true;
}

size_t
co_win_udp_get_receive_data_size(
    const co_udp_t* udp
)
{
    return udp->win.receive.size;
}

void
co_win_udp_set_receive_buffer_size(
    co_udp_t* udp,
    size_t new_size
)
{
    udp->win.receive.new_size = new_size;
}

size_t
co_win_udp_get_receive_buffer_size(
    const co_udp_t* udp
)
{
    return (size_t)udp->win.receive.buffer.len;
}

void*
co_win_udp_get_receive_buffer(
    co_udp_t* udp
)
{
    return &udp->win.receive.buffer.buf[
        udp->win.receive.index];
}

void
co_win_udp_clear_receive_buffer(
    co_udp_t* udp
)
{
    udp->win.receive.index = 0;
    udp->win.receive.size = 0;
}

#endif // CO_OS_WIN
