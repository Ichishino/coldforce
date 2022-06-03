#include <coldforce/core/co_std.h>

#include <coldforce/net/co_tcp_win.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_event.h>

#ifdef CO_OS_WIN

#include <mswsock.h>

//---------------------------------------------------------------------------//
// tcp (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static LPFN_ACCEPTEX co_win_accept_ex = NULL;
static LPFN_CONNECTEX co_win_connect_ex = NULL;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_win_tcp_load_functions(
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
        (LPFN_ACCEPTEX*)&co_win_accept_ex, sizeof(LPFN_ACCEPTEX),
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
        (LPFN_CONNECTEX*)&co_win_connect_ex, sizeof(LPFN_CONNECTEX),
        &bytes, NULL, NULL);

    if (result != 0)
    {
        closesocket(handle);
        return false;
    }

    closesocket(handle);

    return true;
}

co_socket_handle_t
co_win_tcp_socket_create(
    co_address_family_t family
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

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_win_tcp_server_setup(
    co_tcp_server_t* server
)
{
    co_socket_handle_t handle =
        co_win_tcp_socket_create(
            server->sock.local_net_addr.sa.any.ss_family);

    if (handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    server->win.accept_handle = CO_SOCKET_INVALID_HANDLE;
    server->win.accept_io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (server->win.accept_io_ctx == NULL)
    {
        co_socket_handle_close(handle);

        return false;
    }

    server->sock.handle = handle;

    server->win.accept_io_ctx->id = CO_WIN_NET_IO_ID_TCP_ACCEPT;
    server->win.accept_io_ctx->sock = &server->sock;

    return true;
}

void
co_win_tcp_server_cleanup(
    co_tcp_server_t* server
)
{
    co_win_free_io_ctx(server->win.accept_io_ctx);
    server->win.accept_io_ctx = NULL;

    co_socket_handle_close(server->sock.handle);
    server->sock.handle = CO_SOCKET_INVALID_HANDLE;
}

bool
co_win_tcp_server_accept_start(
    co_tcp_server_t* server
)
{
    co_socket_handle_t handle =
        co_win_tcp_socket_create(
            server->sock.local_net_addr.sa.any.ss_family);

    if (handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    server->win.accept_handle = handle;

    memset(&server->win.accept_io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    DWORD addr_storage_size = sizeof(SOCKADDR_STORAGE) + 16;
    DWORD data_size = 0;

    BOOL result = co_win_accept_ex(
        server->sock.handle,
        server->win.accept_handle,
        server->win.buffer,
        0,
        addr_storage_size, addr_storage_size,
        &data_size,
        (LPOVERLAPPED)server->win.accept_io_ctx);

    if (!result)
    {
        int error = co_socket_get_error();

        if (error != ERROR_IO_PENDING)
        {
            return false;
        }
    }

    return true;
}

void
co_win_tcp_server_accept_stop(
    co_tcp_server_t* server
)
{
    co_socket_handle_close(server->win.accept_handle);
    server->win.accept_handle = CO_SOCKET_INVALID_HANDLE;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_win_tcp_client_setup(
    co_tcp_client_t* client,
    size_t receive_buffer_size
)
{
    client->win.io_connect_ctx = NULL;
    client->win.io_send_ctxs = NULL;

    client->win.receive.io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (client->win.receive.io_ctx == NULL)
    {
        return false;
    }

    client->win.receive.io_ctx->id = CO_WIN_NET_IO_ID_TCP_RECEIVE;
    client->win.receive.io_ctx->sock = &client->sock;

    client->win.receive.size = 0;
    client->win.receive.index = 0;
    client->win.receive.new_size = 0;
    client->win.receive.buffer.len = (ULONG)receive_buffer_size;
    client->win.receive.buffer.buf =
        co_mem_alloc(client->win.receive.buffer.len);

    if (client->win.receive.buffer.buf == NULL)
    {
        co_mem_free(client->win.receive.io_ctx);
        client->win.receive.io_ctx = NULL;

        return false;
    }

    return true;
}

void
co_win_tcp_client_cleanup(
    co_tcp_client_t* client
)
{
    if (client->win.receive.io_ctx != NULL)
    {   
        co_win_free_io_ctx(client->win.receive.io_ctx);
        client->win.receive.io_ctx = NULL;
    }

    if (client->win.receive.buffer.buf != NULL)
    {
        co_mem_free_later(client->win.receive.buffer.buf);
        client->win.receive.buffer.buf = NULL;
    }

    if (client->win.io_send_ctxs != NULL)
    {
        co_list_destroy(client->win.io_send_ctxs);
        client->win.io_send_ctxs = NULL;
    }
}

bool
co_win_tcp_client_connector_setup(
    co_tcp_client_t* client,
    const co_net_addr_t* local_net_addr
)
{
    client->sock.handle =
        co_win_tcp_socket_create(
            local_net_addr->sa.any.ss_family);

    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    client->win.io_connect_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (client->win.io_connect_ctx == NULL)
    {
        co_socket_handle_close(client->sock.handle);
        client->sock.handle = CO_SOCKET_INVALID_HANDLE;

        return false;
    }

    client->win.io_connect_ctx->id = CO_WIN_NET_IO_ID_TCP_CONNECT;
    client->win.io_connect_ctx->sock = &client->sock;

    return true;
}

void
co_win_tcp_client_connector_cleanup(
    co_tcp_client_t* client
)
{
    if (client->win.io_connect_ctx != NULL)
    {
        co_win_free_io_ctx(client->win.io_connect_ctx);
        client->win.io_connect_ctx = NULL;
    }
}

bool
co_win_tcp_client_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASend(
        client->sock.handle, &buf, 1,
        &sent_size, 0,
        NULL, NULL);

    return (result == 0);
}

bool
co_win_tcp_client_send_async(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
    if (!client->open_remote)
    {
        return false;
    }

    if (client->win.io_send_ctxs == NULL)
    {
        co_list_ctx_st list_ctx = { 0 };
        list_ctx.free_value = (co_item_free_fn)co_win_free_io_ctx;
        client->win.io_send_ctxs = co_list_create(&list_ctx);
    }

    co_win_net_io_ctx_t* io_ctx =
        (co_win_net_io_ctx_t*)co_mem_alloc(sizeof(co_win_net_io_ctx_t));

    if (io_ctx == NULL)
    {
        return false;
    }

    memset(&io_ctx->ol, 0x00, sizeof(WSAOVERLAPPED));

    io_ctx->id = CO_WIN_NET_IO_ID_TCP_SEND;
    io_ctx->sock = &client->sock;

    WSABUF buf;
    buf.buf = (CHAR*)data;
    buf.len = (ULONG)data_size;

    DWORD sent_size = 0;

    int result = WSASend(
        client->sock.handle, &buf, 1,
        &sent_size, 0,
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
        client->win.io_send_ctxs, (uintptr_t)io_ctx);

    return true;
}

bool
co_win_tcp_client_receive_start(
    co_tcp_client_t* client
)
{
    if (!client->open_remote)
    {
        return false;
    }

    memset(&client->win.receive.io_ctx->ol,
        0x00, sizeof(WSAOVERLAPPED));

    client->win.receive.size = 0;
    client->win.receive.index = 0;

    if (client->win.receive.new_size > 0)
    {
        void* new_buffer =
            co_mem_alloc(client->win.receive.new_size);

        if (new_buffer != NULL)
        {
            co_mem_free(client->win.receive.buffer.buf);
            client->win.receive.buffer.buf = new_buffer;

            client->win.receive.buffer.len =
                (ULONG)client->win.receive.new_size;
        }

        client->win.receive.new_size = 0;
    }

    DWORD flags = 0;
    DWORD data_size = 0;

    int result = WSARecv(
        client->sock.handle,
        &client->win.receive.buffer, 1,
        &data_size,
        &flags,
        (LPWSAOVERLAPPED)client->win.receive.io_ctx,
        NULL);

    if (result != 0)
    {
        int error = co_socket_get_error();

        if (error != ERROR_IO_PENDING)
        {
            co_thread_send_event(client->sock.owner_thread,
                CO_NET_EVENT_ID_TCP_CLOSE, (uintptr_t)client, 0);

            return false;
        }
    }

    return true;
}

bool
co_win_tcp_client_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size,
    size_t* data_size
)
{
    if (client->win.receive.size == 0)
    {
        ssize_t received = co_socket_handle_receive(
            client->sock.handle, buffer, buffer_size, 0);

        if (received > 0)
        {
            *data_size = (size_t)received;

            return true;
        }

        return false;
    }

    *data_size =
        co_min(client->win.receive.size, buffer_size);

    memcpy(buffer,
        &client->win.receive.buffer.buf[client->win.receive.index],
        *data_size);

    client->win.receive.index += *data_size;
    client->win.receive.size -= *data_size;

    return true;
}

bool
co_win_tcp_client_connect_start(
    co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr
)
{
    memset(&client->win.io_connect_ctx->ol,
        0x00, sizeof(WSAOVERLAPPED));

    DWORD sent_size = 0;

    BOOL result = co_win_connect_ex(
        client->sock.handle,
        (const struct sockaddr*)remote_net_addr,
        sizeof(co_net_addr_t),
        NULL, 0, &sent_size,
        (LPOVERLAPPED)client->win.io_connect_ctx);

    if (!result)
    {
        int error = co_socket_get_error();

        if (error != ERROR_IO_PENDING)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

size_t
co_win_tcp_get_receive_data_size(
    const co_tcp_client_t* client
)
{
    return client->win.receive.size;
}

void
co_win_tcp_set_receive_buffer_size(
    co_tcp_client_t* client,
    size_t new_size
)
{
    client->win.receive.new_size = new_size;
}

size_t
co_win_tcp_get_receive_buffer_size(
    const co_tcp_client_t* client
)
{
    return (size_t)client->win.receive.buffer.len;
}

void*
co_win_tcp_get_receive_buffer(
    co_tcp_client_t* client
)
{
    return &client->win.receive.buffer.buf[
        client->win.receive.index];
}

void
co_win_tcp_clear_receive_buffer(
    co_tcp_client_t* client
)
{
    client->win.receive.index = 0;
    client->win.receive.size = 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

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
    co_socket_t* sock,
    co_socket_t* server_sock
)
{
    return co_socket_handle_set_option(sock->handle,
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

#endif // CO_OS_WIN
