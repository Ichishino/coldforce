#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_win.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN

#include <mswsock.h>

#pragma comment(lib, "ws2_32.lib")

//---------------------------------------------------------------------------//
// network (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static LPFN_ACCEPTEX co_win_net_accept_ex = NULL;
static LPFN_CONNECTEX co_win_net_connect_ex = NULL;

static bool
co_win_net_load_functions(
    void
)
{
    int result = 0;
    DWORD bytes = 0;
    SOCKET handle = socket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP);

    GUID accept_ex_guid = WSAID_ACCEPTEX;

    result = WSAIoctl(
        handle,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &accept_ex_guid, sizeof(accept_ex_guid),
        (LPFN_ACCEPTEX*)&co_win_net_accept_ex, sizeof(LPFN_ACCEPTEX),
        &bytes, NULL, NULL);

    if (result != 0)
    {
        closesocket(handle);
        return false;
    }

    GUID connect_ex_guid = WSAID_CONNECTEX;
    bytes = 0;

    result = WSAIoctl(
        handle,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &connect_ex_guid, sizeof(connect_ex_guid),
        (LPFN_CONNECTEX*)&co_win_net_connect_ex, sizeof(LPFN_CONNECTEX),
        &bytes, NULL, NULL);

    if (result != 0)
    {
        closesocket(handle);
        return false;
    }

    closesocket(handle);

    return true;
}

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

    if (!co_win_net_load_functions())
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
co_win_net_client_extension_setup(
    co_win_net_client_extesion_t* win_client,
    co_socket_t* sock,
    size_t receive_buffer_size
)
{
    win_client->io_ctxs = NULL;

    win_client->receive.io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (win_client->receive.io_ctx == NULL)
    {
        return false;
    }

    if (co_socket_type_is_tcp(sock))
    {
        win_client->receive.io_ctx->id = CO_WIN_NET_IO_ID_TCP_RECEIVE;
    }
    else
    {
        win_client->receive.io_ctx->id = CO_WIN_NET_IO_ID_UDP_RECEIVE;
    }

    win_client->receive.io_ctx->sock = sock;

    win_client->receive.size = 0;
    win_client->receive.index = 0;
    win_client->receive.new_size = 0;
    win_client->receive.remote_net_addr = NULL;

    win_client->receive.buffer.len = (ULONG)receive_buffer_size;
    win_client->receive.buffer.buf =
        (CHAR*)co_mem_alloc(win_client->receive.buffer.len);

    if (win_client->receive.buffer.buf == NULL)
    {
        co_mem_free(win_client->receive.io_ctx);
        win_client->receive.io_ctx = NULL;

        return false;
    }

    return true;
}

void
co_win_net_client_extension_cleanup(
    co_win_net_client_extesion_t* win_client
)
{
    if (win_client->receive.io_ctx != NULL)
    {
        co_win_destroy_io_ctx(win_client->receive.io_ctx);
        win_client->receive.io_ctx = NULL;
    }

    if (win_client->receive.buffer.buf != NULL)
    {
        co_mem_free_later(win_client->receive.buffer.buf);
        win_client->receive.buffer.buf = NULL;
    }

    if (win_client->receive.remote_net_addr != NULL)
    {
        co_mem_free(win_client->receive.remote_net_addr);
        win_client->receive.remote_net_addr = NULL;
    }

    if (win_client->io_ctxs != NULL)
    {
        co_list_destroy(win_client->io_ctxs);
        win_client->io_ctxs = NULL;
    }
}

bool
co_win_net_server_extension_setup(
    co_win_net_server_extension_t* win_server,
    co_socket_t* sock
)
{
    win_server->accept.handle = CO_SOCKET_INVALID_HANDLE;
    win_server->accept.io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (win_server->accept.io_ctx == NULL)
    {
        return false;
    }

    win_server->accept.buffer = (uint8_t*)co_mem_alloc(512);

    if (win_server->accept.buffer == NULL)
    {
        co_mem_free(win_server->accept.io_ctx);
        win_server->accept.io_ctx = NULL;

        return false;
    }

    win_server->accept.io_ctx->id = CO_WIN_NET_IO_ID_TCP_ACCEPT;
    win_server->accept.io_ctx->sock = sock;

    return true;
}

void
co_win_net_server_extension_cleanup(
    co_win_net_server_extension_t* win_server
)
{
    if (win_server->accept.io_ctx != NULL)
    {
        co_win_destroy_io_ctx(win_server->accept.io_ctx);
        win_server->accept.io_ctx = NULL;
    }

    if (win_server->accept.handle != CO_SOCKET_INVALID_HANDLE)
    {
        co_socket_handle_close(win_server->accept.handle);
        win_server->accept.handle = CO_SOCKET_INVALID_HANDLE;
    }

    if (win_server->accept.buffer != NULL)
    {
        co_mem_free(win_server->accept.buffer);
        win_server->accept.buffer = NULL;
    }
}

co_socket_handle_t
co_win_socket_handle_create_tcp(
    co_net_addr_family_t family
)
{
    SOCKET sock = WSASocketW(
        family, SOCK_STREAM, IPPROTO_TCP,
        NULL, 0, WSA_FLAG_OVERLAPPED);

#ifdef CO_DEBUG
    if (sock != CO_SOCKET_INVALID_HANDLE)
    {
        CO_DEBUG_SOCKET_COUNTER_INC();
    }
#endif

    return sock;
}

co_socket_handle_t
co_win_socket_handle_create_udp(
    co_net_addr_family_t family
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
co_win_net_connector_extension_setup(
    co_win_net_client_extesion_t* win_client,
    co_socket_t* sock
)
{
    if (win_client->io_ctxs == NULL)
    {
        co_list_ctx_st list_ctx = { 0 };
        list_ctx.destroy_value = (co_item_destroy_fn)co_win_destroy_io_ctx;
        win_client->io_ctxs = co_list_create(&list_ctx);
    }

    co_win_net_io_ctx_t* io_connect_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (io_connect_ctx == NULL)
    {
        return false;
    }

    io_connect_ctx->id = CO_WIN_NET_IO_ID_TCP_CONNECT;
    io_connect_ctx->sock = sock;

    co_list_add_tail(
        win_client->io_ctxs, io_connect_ctx);

    return true;
}

bool
co_win_net_accept_start(
    co_socket_t* sock
)
{
    co_socket_handle_t handle =
        co_win_socket_handle_create_tcp(
            sock->local.net_addr.sa.any.ss_family);

    if (handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    sock->win.server.accept.handle = handle;

    memset(&sock->win.server.accept.io_ctx->ol,
        0x00, sizeof(WSAOVERLAPPED));

    DWORD addr_storage_size = sizeof(SOCKADDR_STORAGE) + 16;
    DWORD data_size = 0;

    BOOL result = co_win_net_accept_ex(
        sock->handle,
        sock->win.server.accept.handle,
        sock->win.server.accept.buffer,
        0,
        addr_storage_size, addr_storage_size,
        &data_size,
        (LPOVERLAPPED)sock->win.server.accept.io_ctx);

    if (!result)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            co_socket_handle_close(
                sock->win.server.accept.handle);
            sock->win.server.accept.handle =
                CO_SOCKET_INVALID_HANDLE;

            return false;
        }
    }

    return true;
}

bool
co_win_net_connect_start(
    co_socket_t* sock,
    const co_net_addr_t* remote_net_addr
)
{
    co_list_data_st* data =
        co_list_get_head(sock->win.client.io_ctxs);

    co_win_net_io_ctx_t* io_connect_ctx =
        (co_win_net_io_ctx_t*)data->value;

    memset(&io_connect_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    DWORD sent_size = 0;

    BOOL result = co_win_net_connect_ex(
        sock->handle,
        (const struct sockaddr*)remote_net_addr,
        sizeof(co_net_addr_t),
        NULL, 0, &sent_size,
        (LPOVERLAPPED)io_connect_ctx);

    if (!result)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            return false;
        }
    }

    return true;
}

bool
co_win_net_send(
    co_socket_t* sock,
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
    const void* data,
    size_t data_size
)
{
    co_win_net_io_ctx_t* io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (io_ctx == NULL)
    {
        return -1;
    }

    memset(&io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    if (co_socket_type_is_tcp(sock))
    {
        io_ctx->id = CO_WIN_NET_IO_ID_TCP_SEND_ASYNC;
    }
    else if (co_socket_type_is_udp(sock))
    {
        io_ctx->id = CO_WIN_NET_IO_ID_UDP_SEND_ASYNC;
    }
    else
    {
        co_mem_free(io_ctx);

        return -1;
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

        if (sock->win.client.io_ctxs == NULL)
        {
            co_list_ctx_st list_ctx = { 0 };
            list_ctx.destroy_value = (co_item_destroy_fn)co_win_destroy_io_ctx;
            sock->win.client.io_ctxs = co_list_create(&list_ctx);
        }

        co_list_add_tail(sock->win.client.io_ctxs, io_ctx);

        return 0;
    }
    else
    {
        co_mem_free(io_ctx);

        co_event_id_t event_id;

        if (co_socket_type_is_tcp(sock))
        {
            event_id = CO_NET_EVENT_ID_TCP_SEND_ASYNC_COMPLETE;
        }
        else
        {
            event_id = CO_NET_EVENT_ID_UDP_SEND_ASYNC_COMPLETE;
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
    co_socket_t* sock
)
{
    if (sock->handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    memset(&sock->win.client.receive.io_ctx->ol,
        0x00, sizeof(WSAOVERLAPPED));

    sock->win.client.receive.size = 0;
    sock->win.client.receive.index = 0;

    if (sock->win.client.receive.new_size > 0)
    {
        void* new_buffer =
            co_mem_alloc(sock->win.client.receive.new_size);

        if (new_buffer != NULL)
        {
            co_mem_free(sock->win.client.receive.buffer.buf);
            sock->win.client.receive.buffer.buf = new_buffer;

            sock->win.client.receive.buffer.len =
                (ULONG)sock->win.client.receive.new_size;
        }

        sock->win.client.receive.new_size = 0;
    }

    DWORD flags = 0;
    DWORD data_size = 0;

    int result = WSARecv(
        sock->handle,
        &sock->win.client.receive.buffer, 1,
        &data_size,
        &flags,
        (LPWSAOVERLAPPED)sock->win.client.receive.io_ctx,
        NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            if (co_socket_type_is_tcp(sock))
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

        if (co_socket_type_is_tcp(sock))
        {
            event_id = CO_NET_EVENT_ID_TCP_RECEIVE_READY;
        }
        else
        {
            event_id = CO_NET_EVENT_ID_UDP_RECEIVE_READY;
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
    void* buffer,
    size_t buffer_size
)
{
    if (sock->win.client.receive.size == 0)
    {
        return co_socket_handle_receive(
            sock->handle, buffer, buffer_size, 0);
    }

    size_t data_size =
        co_min(sock->win.client.receive.size, buffer_size);

    memcpy(buffer,
        &sock->win.client.receive.buffer.buf[
            sock->win.client.receive.index],
        data_size);

    sock->win.client.receive.index += data_size;
    sock->win.client.receive.size -= data_size;

    return (ssize_t)data_size;
}

bool
co_win_net_send_to(
    co_socket_t* sock,
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
        sock->handle, &buf, 1,
        &sent_size, 0,
        (const struct sockaddr*)remote_net_addr,
        sizeof(co_net_addr_t),
        NULL, NULL);

    return (result == 0);
}

ssize_t
co_win_net_send_to_async(
    co_socket_t* sock,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
)
{
    co_win_net_io_ctx_t* io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (io_ctx == NULL)
    {
        return -1;
    }

    memset(&io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    io_ctx->id = CO_WIN_NET_IO_ID_UDP_SEND_ASYNC;
    io_ctx->sock = sock;

    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASendTo(
        sock->handle, &buf, 1,
        &sent_size, 0,
        (const struct sockaddr*)remote_net_addr,
        sizeof(co_net_addr_t),
        (LPWSAOVERLAPPED)io_ctx, NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            co_mem_free(io_ctx);

            return -1;
        }

        if (sock->win.client.io_ctxs == NULL)
        {
            co_list_ctx_st list_ctx = { 0 };
            list_ctx.destroy_value = (co_item_destroy_fn)co_win_destroy_io_ctx;
            sock->win.client.io_ctxs = co_list_create(&list_ctx);
        }

        co_list_add_tail(sock->win.client.io_ctxs, io_ctx);

        return 0;
    }
    else
    {
        co_mem_free(io_ctx);

        co_thread_send_event(
            sock->owner_thread,
            CO_NET_EVENT_ID_UDP_SEND_ASYNC_COMPLETE,
            (uintptr_t)sock,
            (uintptr_t)data_size);

        return (ssize_t)data_size;
    }
}

bool
co_win_net_receive_from_start(
    co_socket_t* sock
)
{
    if (sock->handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    memset(&sock->win.client.receive.io_ctx->ol,
        0x00, sizeof(WSAOVERLAPPED));

    sock->win.client.receive.size = 0;
    sock->win.client.receive.index = 0;

    if (sock->win.client.receive.new_size > 0)
    {
        void* new_buffer =
            co_mem_alloc(sock->win.client.receive.new_size);

        if (new_buffer != NULL)
        {
            co_mem_free(sock->win.client.receive.buffer.buf);
            sock->win.client.receive.buffer.buf = new_buffer;

            sock->win.client.receive.buffer.len =
                (ULONG)sock->win.client.receive.new_size;
        }

        sock->win.client.receive.new_size = 0;
    }

    DWORD flags = 0;
    DWORD data_size = 0;
    INT sender_net_addr_size = sizeof(co_net_addr_t);

    int result = WSARecvFrom(
        sock->handle,
        &sock->win.client.receive.buffer, 1,
        &data_size,
        &flags,
        (struct sockaddr*)sock->win.client.receive.remote_net_addr,
        &sender_net_addr_size,
        (LPWSAOVERLAPPED)sock->win.client.receive.io_ctx,
        NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != WSA_IO_PENDING)
        {
            return false;
        }
    }
    else
    {
        co_thread_send_event(
            sock->owner_thread,
            CO_NET_EVENT_ID_UDP_RECEIVE_READY,
            (uintptr_t)sock,
            (uintptr_t)data_size);
    }

    return true;
}

ssize_t
co_win_net_receive_from(
    co_socket_t* sock,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size
)
{
    if (sock->win.client.receive.size == 0)
    {
        return co_socket_handle_receive_from(
            sock->handle, remote_net_addr, buffer, buffer_size, 0);
    }

    if (remote_net_addr != NULL)
    {
        memcpy(remote_net_addr,
            sock->win.client.receive.remote_net_addr,
            sizeof(co_net_addr_t));
    }

    ssize_t data_size =
        (ssize_t)co_min(sock->win.client.receive.size, buffer_size);

    memcpy(buffer,
        &sock->win.client.receive.buffer.buf[
            sock->win.client.receive.index],
        data_size);

    sock->win.client.receive.index += data_size;
    sock->win.client.receive.size -= data_size;

    return data_size;
}

bool
co_win_socket_option_update_connect_context(
    co_socket_t* sock
)
{
    return co_socket_handle_set_option(sock->handle,
        SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
}

bool
co_win_socket_option_update_accept_context(
    co_socket_t* server_sock,
    co_socket_handle_t accept_handle
)
{
    return co_socket_handle_set_option(accept_handle,
        SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
        &server_sock->handle, sizeof(co_socket_handle_t));
}

bool
co_win_socket_option_get_connect_time(
    co_socket_t* sock,
    int* seconds
)
{
    size_t value_size = sizeof(int);

    return co_socket_handle_get_option(sock->handle,
        SOL_SOCKET, SO_CONNECT_TIME,
        seconds, &value_size);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

size_t
co_win_socket_get_receive_data_size(
    const co_socket_t* sock
)
{
    return sock->win.client.receive.size;
}

void
co_win_socket_set_receive_buffer_size(
    co_socket_t* sock,
    size_t new_size
)
{
    sock->win.client.receive.new_size = new_size;
}

size_t
co_win_socket_get_receive_buffer_size(
    const co_socket_t* sock
)
{
    return (size_t)sock->win.client.receive.buffer.len;
}

const void*
co_win_socket_get_receive_buffer(
    const co_socket_t* sock
)
{
    return &sock->win.client.receive.buffer.buf[
        sock->win.client.receive.index];
}

void
co_win_socket_clear_receive_buffer(
    co_socket_t* sock
)
{
    sock->win.client.receive.index = 0;
    sock->win.client.receive.size = 0;
}

#endif // CO_OS_WIN
