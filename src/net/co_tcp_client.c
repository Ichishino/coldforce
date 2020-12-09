#include <coldforce/core/co_std.h>

#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_worker.h>

#ifndef CO_OS_WIN
#include <errno.h>
#endif

//---------------------------------------------------------------------------//
// tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_tcp_client_t*
co_tcp_client_create_with(
    co_socket_handle_t handle,
    const co_net_addr_t* remote_net_addr
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

    client->sock.handle = handle;
    client->sock.type = CO_SOCKET_TYPE_TCP_CONNECTION;
    client->sock.open_local = true;
    client->sock.sub_class = NULL;
    client->open_remote = true;

    co_socket_handle_get_local_net_addr(
        client->sock.handle, &client->sock.local_net_addr);

    if (remote_net_addr != NULL)
    {
        memcpy(&client->remote_net_addr,
            remote_net_addr, sizeof(co_net_addr_t));
    }
    else
    {
        co_socket_handle_get_remote_net_addr(
            client->sock.handle, &client->remote_net_addr);
    }

    return client;
}

bool
co_tcp_client_setup(
    co_tcp_client_t* client
)
{
    co_net_addr_init(&client->sock.local_net_addr);
    co_net_addr_init(&client->remote_net_addr);

    client->sock.type = 0;
    client->sock.owner_thread = NULL;
    client->sock.handle = CO_SOCKET_INVALID_HANDLE;

    client->sock.open_local = false;
    client->open_remote = false;

    client->on_connect_complete = NULL;
    client->on_send_complete = NULL;
    client->on_receive_ready = NULL;
    client->on_close = NULL;

    client->close_timer = NULL;
    client->tls = NULL;

#ifdef CO_OS_WIN

    if (!co_win_tcp_client_setup(
        client, CO_WIN_TCP_DEFAULT_RECEIVE_BUFFER_SIZE))
    {
        return false;
    }

#else
    client->send_queue = NULL;
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
#else

    if (client->send_queue != NULL)
    {
        co_buffer_st data;

        while (co_queue_pop(client->send_queue, &data))
        {
            co_mem_free(data.ptr);
        }

        co_queue_destroy(client->send_queue);
        client->send_queue = NULL;
    }

#endif

    client->on_connect_complete = NULL;
    client->on_send_complete = NULL;
    client->on_receive_ready = NULL;
    client->on_close = NULL;

    co_socket_handle_close(client->sock.handle);
    client->sock.handle = CO_SOCKET_INVALID_HANDLE;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_tcp_client_on_connect_complete(
    co_tcp_client_t* client,
    int error_code
)
{
    if (error_code == 0)
    {
        client->sock.type = CO_SOCKET_TYPE_TCP_CONNECTION;
        client->open_remote = true;

#ifdef CO_OS_WIN

        co_win_socket_option_update_connect_context(&client->sock);
        co_win_tcp_client_receive_start(client);

#else

        co_net_worker_unregister_tcp_connector(
            co_socket_get_net_worker(&client->sock),
            client);

        co_net_worker_register_tcp_connection(
            co_socket_get_net_worker(&client->sock),
            client);
#endif

        co_socket_handle_get_remote_net_addr(
            client->sock.handle, &client->remote_net_addr);
    }

    if (client->on_connect_complete != NULL)
    {
        client->on_connect_complete(
            client->sock.owner_thread, client, error_code);
    }
}

void
co_tcp_client_on_send_ready(
    co_tcp_client_t* client
)
{
#ifdef CO_OS_WIN
    (void)client;
#else

    co_buffer_st* send_data =
        (co_buffer_st*)co_queue_peek_head(client->send_queue);

    if (send_data == NULL)
    {
        co_net_worker_set_tcp_send(
            co_socket_get_net_worker(&client->sock),
            client, false);

        return;
    }

    ssize_t sent_size = co_socket_handle_send(
        client->sock.handle, send_data->ptr, send_data->size, 0);

    if (sent_size == send_data->size)
    {
        co_buffer_st used_data;
        co_queue_pop(client->send_queue, &used_data);

        co_mem_free(used_data.ptr);

        co_event_send(client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_SEND_COMPLETE,
            (uintptr_t)client, (uintptr_t)sent_size);

        co_tcp_client_on_send_ready(client);
    }

#endif
}

void
co_tcp_client_on_send_complete(
    co_tcp_client_t* client,
    size_t data_size
)
{
#ifdef CO_OS_WIN
    co_list_remove_head(client->win.io_send_ctxs);
#endif

    if (client->on_send_complete != NULL)
    {
        client->on_send_complete(
            client->sock.owner_thread, client, (data_size > 0));
    }
}

void
co_tcp_client_on_receive_ready(
    co_tcp_client_t* client,
    size_t data_size
)
{
#ifdef CO_OS_WIN
    if (data_size > 0)
    {
        client->win.receive.size = data_size;
    }
#endif

    if ((client->on_receive_ready != NULL) && client->sock.open_local)
    {
        client->on_receive_ready(
            client->sock.owner_thread, client);
    }
#ifdef CO_OS_WIN
    else
    {
        client->win.receive.index = 0;
        client->win.receive.size = 0;
    }

    if (client->win.receive.size == 0)
    {
        co_win_tcp_client_receive_start(client);
    }
    else
    {
        co_event_send(client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_RECEIVE_READY,
            (uintptr_t)client, 0);
    }
#endif
}

void
co_tcp_client_on_close(
    co_tcp_client_t* client
)
{
    if (!co_net_worker_close_tcp_client_remote(
        co_socket_get_net_worker(&client->sock), client))
    {
        return;
    }

    if (client->sock.open_local)
    {
        client->sock.open_local = false;

        if (client->on_close != NULL)
        {
            client->on_close(client->sock.owner_thread, client);
        }
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

    client->sock.type = CO_SOCKET_TYPE_TCP_CONNECTOR;
    client->sock.owner_thread = co_thread_get_current();
    client->sock.sub_class = NULL;
    client->sock.open_local = true;

    memcpy(&client->sock.local_net_addr,
        local_net_addr, sizeof(co_net_addr_t));

#ifdef CO_OS_WIN
    co_win_tcp_client_connector_setup(client, &client->sock.local_net_addr);
#else
    client->sock.handle = co_socket_handle_create(
        client->sock.local_net_addr.sa.any.ss_family, SOCK_STREAM, IPPROTO_TCP);
#endif

    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
#ifdef CO_OS_WIN
        co_win_tcp_client_connector_cleanup(client);
#endif
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

        return NULL;
    }

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
        co_mem_free_later(client);
    }
}

int
co_tcp_connect(
    co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr
)
{
    co_socket_handle_set_blocking(client->sock.handle, true);

    if (!co_socket_handle_connect(
        client->sock.handle, remote_net_addr))
    {
        return co_socket_get_error();
    }

    co_socket_handle_get_remote_net_addr(
        client->sock.handle, &client->remote_net_addr);

    co_net_worker_register_tcp_connection(
        co_socket_get_net_worker(&client->sock),
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
    const co_net_addr_t* remote_net_addr,
    co_tcp_connect_fn handler
)
{
    co_net_worker_register_tcp_connector(
        co_socket_get_net_worker(&client->sock),
        client);

    client->on_connect_complete = handler;

#ifdef CO_OS_WIN

    if (!co_win_tcp_client_connect_start(client, remote_net_addr))
    {
        return false;
    }

#else

    if (co_socket_handle_connect(
        client->sock.handle, remote_net_addr))
    {
        co_event_send(client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_CONNECT_COMPLETE, (uintptr_t)client, 0);
    }
    else
    {
        int error_code = co_socket_get_error();

        if (error_code != EINPROGRESS)
        {
            return false;
        }
    }

#endif

    return true;
}

bool
co_tcp_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
#ifdef CO_OS_WIN

    return co_win_tcp_client_send(client, data, data_size);

#else

    co_socket_handle_set_blocking(client->sock.handle, true);

    ssize_t sent_size =
        co_socket_handle_send(
            client->sock.handle, data, data_size, 0);

    co_socket_handle_set_blocking(client->sock.handle, false);

    return (data_size == (size_t)sent_size);

#endif
}

bool
co_tcp_send_async(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
#ifdef CO_OS_WIN

    return co_win_tcp_client_send_async(client, data, data_size);

#else

    co_net_worker_t* net_worker =
        co_socket_get_net_worker(&client->sock);

    co_net_worker_set_tcp_send(net_worker, client, true);

    if (co_socket_handle_send(
        client->sock.handle, data, data_size, 0) == (ssize_t)data_size)
    {
        co_net_worker_set_tcp_send(net_worker, client, false);

        co_event_send(client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_SEND_COMPLETE, (uintptr_t)client, (uintptr_t)data_size);

        return true;
    }
    else
    {
        int error_code = co_socket_get_error();

        if ((error_code == EAGAIN) || (error_code == EWOULDBLOCK))
        {
            if (client->send_queue == NULL)
            {
                client->send_queue = co_queue_create(sizeof(co_buffer_st), NULL);
            }

            co_buffer_st buffer;

            buffer.size = data_size;
            buffer.ptr = co_mem_alloc(buffer.size);

            if (buffer.ptr == NULL)
            {
                co_net_worker_set_tcp_send(net_worker, client, false);

                return false;
            }

            memcpy(buffer.ptr, data, buffer.size);

            co_queue_push(client->send_queue, &buffer);
        }
        else
        {
            co_net_worker_set_tcp_send(net_worker, client, false);

            return false;
        }
    }

    return true;

#endif
}

ssize_t
co_tcp_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size
)
{
#ifdef CO_OS_WIN

    size_t data_size = 0;

    if (co_win_tcp_client_receive(
        client, buffer, buffer_size, &data_size))
    {
        return (ssize_t)data_size;
    }
    else
    {
        return -1;
    }

#else

    ssize_t data_size =
        co_socket_handle_receive(
            client->sock.handle, buffer, buffer_size, 0);

    return data_size;

#endif
}

ssize_t
co_tcp_receive_all(
    co_tcp_client_t* client,
    co_byte_array_t* byte_array
)
{
    ssize_t index = 0;

    for (;;)
    {
        char buffer[8192];

        ssize_t size = co_tcp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        co_byte_array_add(byte_array, buffer, size);

        index += size;
    }

    return index;
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
            co_socket_get_net_worker(&client->sock), client);
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
co_tcp_set_send_complete_handler(
    co_tcp_client_t* client,
    co_tcp_send_fn handler
)
{
    client->on_send_complete = handler;
}

void
co_tcp_set_receive_handler(
    co_tcp_client_t* client,
    co_tcp_receive_fn handler
)
{
    client->on_receive_ready = handler;
}

void
co_tcp_set_close_handler(
    co_tcp_client_t* client,
    co_tcp_close_fn handler
)
{
    client->on_close = handler;
}
