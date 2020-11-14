#include <coldforce/core/co_std.h>

#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_tcp_client_setup(
    co_tcp_client_t* client
)
{
    co_net_addr_init(&client->sock.local_net_addr);
    co_net_addr_init(&client->remote_net_addr);

    client->sock.owner_thread = NULL;
    client->sock.handle = CO_SOCKET_INVALID_HANDLE;

    client->sock.open_local = false;
    client->open_remote = false;

    client->on_connect = NULL;
    client->on_send = NULL;
    client->on_receive = NULL;
    client->on_close = NULL;

    client->close_timer = NULL;

#ifdef CO_OS_WIN
    if (!co_win_tcp_client_setup(
        client, CO_WIN_TCP_DEFAULT_RECEIVE_BUFF_LENGTH))
    {
        return false;
    }
#endif

    return true;
}

void
co_tcp_client_cleanup(
    co_tcp_client_t* client
)
{
#ifdef CO_OS_WIN
    co_win_tcp_client_connector_cleanup(client);
    co_win_tcp_client_cleanup(client);
#endif

    co_socket_handle_close(client->sock.handle);
    client->sock.handle = CO_SOCKET_INVALID_HANDLE;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_tcp_client_on_connect(
    co_tcp_client_t* client,
    int error_code
)
{
#ifdef CO_OS_WIN
    if (error_code == 0)
    {
        co_win_socket_option_update_connect_context(&client->sock);
        co_win_tcp_client_receive_start(client);
    }
#endif

    if (error_code == 0)
    {
        client->open_remote = true;

        co_net_worker_register_tcp_connection(
            (co_net_worker_t*)client->sock.owner_thread->event_worker,
            client);
    }

    co_assert(client->on_connect != NULL);

    client->on_connect(client->sock.owner_thread, client, error_code);
}

void
co_tcp_client_on_send(
    co_tcp_client_t* client,
    size_t data_length
)
{
#ifdef CO_OS_WIN
    co_list_remove_head(client->win.io_send_ctxs);
#endif

    if (client->on_send != NULL)
    {
        client->on_send(
            client->sock.owner_thread, client, (data_length > 0));
    }
}

void
co_tcp_client_on_receive(
    co_tcp_client_t* client,
    size_t data_length
)
{
#ifdef CO_OS_WIN
    client->win.receive.length = data_length;
#endif

    co_assert(client->on_receive != NULL);

    if (client->sock.open_local)
    {
        client->on_receive(client->sock.owner_thread, client);
    }
#ifdef CO_OS_WIN
    else
    {
        client->win.receive.index = 0;
        client->win.receive.length = 0;
    }

    if (client->win.receive.length == 0)
    {
        co_win_tcp_client_receive_start(client);
    }
    else
    {
        co_event_send(client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_RECEIVE,
            (uintptr_t)client, (uintptr_t)client->win.receive.length);
    }
#endif
}

void
co_tcp_client_on_close(
    co_tcp_client_t* client
)
{
    co_net_worker_close_tcp_client_remote(
        (co_net_worker_t*)client->sock.owner_thread->event_worker, client);

    if (client->sock.open_local)
    {
        client->sock.open_local = false;

        co_assert(client->on_close != NULL);

        client->on_close(client->sock.owner_thread, client);
    }
    else
    {
        co_tcp_client_destroy(client);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_tcp_client_t*
co_tcp_client_create(
    const co_net_addr_t* remote_net_addr,
    const co_net_addr_t* local_net_addr
)
{
    co_tcp_client_t* client =
        (co_tcp_client_t*)co_mem_alloc(sizeof(co_tcp_client_t));

    if (client == NULL)
    {
        return NULL;
    }

    if (!co_tcp_client_setup(client))
    {
        co_mem_free(client);

        return NULL;
    }

    client->sock.owner_thread = co_thread_get_current();

    memcpy(&client->remote_net_addr,
        remote_net_addr, sizeof(co_net_addr_t));

    if (local_net_addr != NULL)
    {
        memcpy(&client->sock.local_net_addr,
            local_net_addr, sizeof(co_net_addr_t));
    }
    else
    {
        client->sock.local_net_addr.sa.any.ss_family =
            client->remote_net_addr.sa.any.ss_family;
    }

#ifdef CO_OS_WIN
    co_win_tcp_client_connector_setup(client, &client->remote_net_addr);
#else

#endif

    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        co_win_tcp_client_connector_cleanup(client);
        co_mem_free(client);

        return NULL;
    }

    if (!co_socket_handle_bind(
        client->sock.handle,
        &client->sock.local_net_addr))
    {
        co_socket_handle_close(client->sock.handle);
        client->sock.handle = CO_SOCKET_INVALID_HANDLE;

        co_tcp_client_cleanup(client);
        co_mem_free(client);

        return false;
    }

    client->sock.open_local = true;

    return client;
}

void
co_tcp_client_destroy(
    co_tcp_client_t* client
)
{
    if (client == NULL)
    {
        return;
    }

    co_tcp_client_close(client);

    if (!client->sock.open_local && !client->open_remote)
    {
        co_tcp_client_cleanup(client);
        co_mem_free(client);
    }
}

int
co_tcp_connect(
    co_tcp_client_t* client
)
{
    if (!co_socket_handle_connect(
        client->sock.handle, &client->remote_net_addr))
    {
        return co_socket_get_error();
    }

    co_net_worker_register_tcp_connection(
        (co_net_worker_t*)client->sock.owner_thread->event_worker,
        client);

#ifdef CO_OS_WIN
    client->open_remote = true;
    co_win_tcp_client_receive_start(client);
#endif

    return 0;
}

bool
co_tcp_connect_async(
    co_tcp_client_t* client,
    co_tcp_connect_fn handler
)
{
    co_net_worker_register_tcp_connector(
        (co_net_worker_t*)client->sock.owner_thread->event_worker,
        client);

#ifdef CO_OS_WIN
    if (!co_win_tcp_client_connect_start(client))
    {
        return false;
    }
#else

#endif

    client->on_connect = handler;

    return true;
}

bool
co_tcp_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_length
)
{
#ifdef CO_OS_WIN

    if (!co_win_tcp_client_send(client, data, data_length))
    {
        return false;
    }

#else

#endif
    return true;
}

bool
co_tcp_send_string(
    co_tcp_client_t* client,
    const char* data
)
{
    return co_tcp_send(client, data, strlen(data) + 1);
}

bool
co_tcp_send_async(
    co_tcp_client_t* client,
    const void* data,
    size_t data_length
)
{
#ifdef CO_OS_WIN

    if (!co_win_tcp_client_send_async(client, data, data_length))
    {
        return false;
    }

#else

#endif

    return true;
}

ssize_t
co_tcp_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_length
)
{
#ifdef CO_OS_WIN

    size_t received_length = 0;

    if (!co_win_tcp_client_receive(
        client, buffer, buffer_length, &received_length))
    {
        return -1;
    }
    else
    {
        return (ssize_t)received_length;
    }

#else
    return 0;
#endif
}

void
co_tcp_client_close(
    co_tcp_client_t* client
)
{
    if (client == NULL)
    {
        return;
    }

    if (client->sock.owner_thread != NULL)
    {
        co_net_worker_close_tcp_client_local(
            (co_net_worker_t*)client->sock.owner_thread->event_worker, client);
    }
    else
    {
        client->sock.open_local = false;
        client->open_remote = false;

        co_socket_handle_close(client->sock.handle);
        client->sock.handle = CO_SOCKET_INVALID_HANDLE;
    }
}

const co_net_addr_t*
co_tcp_get_remote_net_addr(
    const co_tcp_client_t* client
)
{
    return &client->remote_net_addr;
}

bool
co_tcp_is_open(
    const co_tcp_client_t* client
)
{
    return (client->sock.open_local && client->open_remote);
}

void
co_tcp_set_send_handler(
    co_tcp_client_t* client,
    co_tcp_send_fn handler
)
{
    client->on_send = handler;
}

void
co_tcp_set_receive_handler(
    co_tcp_client_t* client,
    co_tcp_receive_fn handler
)
{
    client->on_receive = handler;
}

void
co_tcp_set_close_handler(
    co_tcp_client_t* client,
    co_tcp_close_fn handler
)
{
    client->on_close = handler;
}
