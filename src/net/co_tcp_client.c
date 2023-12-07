#include <coldforce/core/co_std.h>

#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_log.h>

#ifndef CO_OS_WIN
#include <errno.h>
#endif

//---------------------------------------------------------------------------//
// tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
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

    if (!co_tcp_client_setup(
        client, CO_SOCKET_TYPE_TCP))
    {
        co_mem_free(client);

        return NULL;
    }

    client->sock.handle = handle;
    client->sock.local.is_open = true;
    client->sock.remote.is_open = true;

    co_socket_handle_get_local_net_addr(
        client->sock.handle, &client->sock.local.net_addr);

    if (remote_net_addr != NULL)
    {
        memcpy(&client->sock.remote.net_addr,
            remote_net_addr, sizeof(co_net_addr_t));
    }
    else
    {
        co_socket_handle_get_remote_net_addr(
            client->sock.handle, &client->sock.remote.net_addr);
    }

    return client;
}

bool
co_tcp_client_setup(
    co_tcp_client_t* client,
    co_socket_type_t type
)
{
    co_socket_setup(&client->sock, type);

    client->send_async_queue = NULL;

    client->callbacks.on_connect = NULL;
    client->callbacks.on_send_async = NULL;
    client->callbacks.on_receive = NULL;
    client->callbacks.on_receive_timer = NULL;
    client->callbacks.on_close = NULL;

    client->close_timer = NULL;

#ifdef CO_OS_WIN
    if (!co_win_net_client_extension_setup(
        &client->sock.win.client, &client->sock,
        CO_WIN_TCP_DEFAULT_RECEIVE_BUFFER_SIZE))
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
    co_win_net_client_extension_cleanup(
        &client->sock.win.client);
#endif

    if (client->send_async_queue != NULL)
    {
        co_queue_destroy(client->send_async_queue);
        client->send_async_queue = NULL;
    }

    client->callbacks.on_connect = NULL;
    client->callbacks.on_send_async = NULL;
    client->callbacks.on_receive = NULL;
    client->callbacks.on_close = NULL;

    co_socket_cleanup(&client->sock);
}

void
co_tcp_client_on_connect_complete(
    co_tcp_client_t* client,
    int error_code
)
{
    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_socket_handle_get_local_net_addr(
        client->sock.handle, &client->sock.local.net_addr);

    if (error_code == 0)
    {
        client->sock.type = CO_SOCKET_TYPE_TCP;
        client->sock.remote.is_open = true;

#ifdef CO_OS_WIN

        co_win_socket_option_update_connect_context(&client->sock);
        co_win_net_receive_start(&client->sock);

#else

        co_net_worker_unregister_tcp_connector(
            co_socket_get_net_worker(&client->sock),
            client);

        co_net_worker_register_tcp_connection(
            co_socket_get_net_worker(&client->sock),
            client);
#endif

        co_socket_handle_get_remote_net_addr(
            client->sock.handle, &client->sock.remote.net_addr);
    }

    if (error_code == 0)
    {
        co_tcp_log_info(
            &client->sock.local.net_addr,
            "<--",
            &client->sock.remote.net_addr,
            "tcp connect success");
    }
    else
    {
        co_tcp_log_error(
            &client->sock.local.net_addr,
            "<--",
            &client->sock.remote.net_addr,
            "tcp connect error (%d)", error_code);
    }

    if (client->callbacks.on_connect != NULL)
    {
        client->callbacks.on_connect(
            client->sock.owner_thread, client, error_code);
    }
}

#ifndef CO_OS_WIN
void
co_tcp_client_on_send_async_ready(
    co_tcp_client_t* client
)
{
    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_tcp_send_async_data_t* send_data =
        (co_tcp_send_async_data_t*)co_queue_peek_head(client->send_async_queue);

    if (send_data == NULL)
    {
        co_net_worker_set_tcp_send(
            co_socket_get_net_worker(&client->sock),
            client, false);

        return;
    }

    co_tcp_log_debug(
        &client->sock.local.net_addr,
        "<--",
        &client->sock.remote.net_addr,
        "tcp send async ready");

    ssize_t sent_size = co_socket_handle_send(
        client->sock.handle, send_data->data, send_data->data_size, 0);

    if ((size_t)sent_size == send_data->data_size)
    {
        co_tcp_client_on_send_async_complete(client, true);
        co_tcp_client_on_send_async_ready(client);
    }
    else
    {
        co_tcp_log_debug(
            &client->sock.local.net_addr,
            NULL,
            NULL,
            "tcp send async QUEUED %zd items",
            co_queue_get_count(client->send_async_queue));
    }
}
#endif // !CO_OS_WIN

void
co_tcp_client_on_send_async_complete(
    co_tcp_client_t* client,
    bool result
)
{
    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_tcp_send_async_data_t send_data;

    if ((client->send_async_queue == NULL) ||
        !co_queue_pop(client->send_async_queue, &send_data))
    {
        return;
    }

    co_tcp_log_debug_hex_dump(
        &client->sock.local.net_addr,
        "-->",
        &client->sock.remote.net_addr,
        send_data.data, send_data.data_size,
        "tcp send async %d bytes", send_data.data_size);

    if (client->callbacks.on_send_async != NULL)
    {
        client->callbacks.on_send_async(
            client->sock.owner_thread, client,
            send_data.user_data, result);
    }
}

void
co_tcp_client_on_receive_ready(
    co_tcp_client_t* client,
    size_t data_size
)
{
    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_tcp_log_debug(
        &client->sock.local.net_addr,
        "<--",
        &client->sock.remote.net_addr,
        "tcp receive ready");

#ifdef CO_OS_WIN

    if (data_size > 0)
    {
        client->sock.win.client.receive.size = data_size;
    }

    if (client->callbacks.on_receive != NULL)
    {
        client->callbacks.on_receive(
            client->sock.owner_thread, client);
    }
    else
    {
        client->sock.win.client.receive.index = 0;
        client->sock.win.client.receive.size = 0;
    }

    if (data_size > 0)
    {
        if (client->sock.win.client.receive.size == 0)
        {
            co_win_net_receive_start(&client->sock);
        }
        else
        {
            co_thread_send_event(
                client->sock.owner_thread,
                CO_NET_EVENT_ID_TCP_RECEIVE_READY,
                (uintptr_t)client,
                0);
        }
    }

#else

    (void)data_size;

    if (client->callbacks.on_receive != NULL)
    {
        client->callbacks.on_receive(
            client->sock.owner_thread, client);
    }

#endif // CO_OS_WIN
}

static void
co_tcp_client_on_receive_timer(
    co_thread_t* thread,
    co_timer_t* timer
)
{
    co_tcp_client_t* client =
        (co_tcp_client_t*)co_timer_get_user_data(timer);

    if (client->callbacks.on_receive_timer != NULL)
    {
        client->callbacks.on_receive_timer(thread, client);
    }
}

void
co_tcp_client_on_close(
    co_tcp_client_t* client
)
{
    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_net_worker_close_tcp_client_remote(
        co_socket_get_net_worker(&client->sock), client);

    co_tcp_log_info(
        &client->sock.local.net_addr,
        "<--",
        &client->sock.remote.net_addr,
        "tcp closed by peer");

    client->sock.local.is_open = false;

    if (client->callbacks.on_close != NULL)
    {
        client->callbacks.on_close(client->sock.owner_thread, client);
    }
    else
    {
        co_tcp_client_destroy(client);
    }
}

//---------------------------------------------------------------------------//
// public
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

    if (!co_tcp_client_setup(
        client, CO_SOCKET_TYPE_TCP_CONNECTOR))
    {
        co_tcp_client_cleanup(client);
        co_mem_free(client);

        return NULL;
    }

    client->sock.owner_thread = co_thread_get_current();
    client->sock.local.is_open = true;

    memcpy(&client->sock.local.net_addr,
        local_net_addr, sizeof(co_net_addr_t));

#ifdef CO_OS_WIN

    if (!co_win_net_connector_extension_setup(
        &client->sock.win.client, &client->sock))
    {
        co_tcp_client_cleanup(client);
        co_mem_free(client);

        return NULL;
    }

    client->sock.handle = co_win_socket_handle_create_tcp(
        client->sock.local.net_addr.sa.any.ss_family);
#else
    client->sock.handle = co_socket_handle_create(
        client->sock.local.net_addr.sa.any.ss_family, SOCK_STREAM, 0);
#endif

    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        co_tcp_client_cleanup(client);
        co_mem_free(client);

        return NULL;
    }

    if (!co_socket_handle_bind(
        client->sock.handle,
        &client->sock.local.net_addr))
    {
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

    if (client->sock.owner_thread != NULL)
    {
        client->callbacks.on_connect = NULL;
        client->callbacks.on_send_async = NULL;
        client->callbacks.on_receive = NULL;
        client->callbacks.on_receive_timer = NULL;
        client->callbacks.on_close = NULL;

        co_net_worker_close_tcp_client_local(
            co_socket_get_net_worker(&client->sock),
            client, 3*1000);
    }

    if (!client->sock.local.is_open && !client->sock.remote.is_open)
    {
        co_tcp_close(client);
        co_tcp_client_cleanup(client);
        co_mem_free_later(client);
    }
}

bool
co_tcp_connect_start(
    co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr
)
{
    co_net_worker_register_tcp_connector(
        co_socket_get_net_worker(&client->sock),
        client);

#ifdef CO_OS_WIN

    if (!co_win_net_connect_start(
        &client->sock, remote_net_addr))
    {
        return false;
    }

#else

    if (co_socket_handle_connect(
        client->sock.handle, remote_net_addr))
    {
        co_thread_send_event(
            client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_CONNECT_COMPLETE,
            (uintptr_t)client,
            0);
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

    co_socket_handle_get_local_net_addr(
        client->sock.handle, &client->sock.local.net_addr);

    co_tcp_log_info(
        &client->sock.local.net_addr,
        "-->",
        remote_net_addr,
        "tcp connect start");

    return true;
}

bool
co_tcp_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
)
{
    co_tcp_log_debug_hex_dump(
        &client->sock.local.net_addr,
        "-->",
        &client->sock.remote.net_addr,
        data, data_size,
        "tcp send %zd bytes", data_size);

#ifdef CO_OS_WIN

    return co_win_net_send(
        &client->sock, data, data_size);

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
    size_t data_size,
    void* user_data
)
{
    if (client->send_async_queue == NULL)
    {
        client->send_async_queue = co_queue_create(
            sizeof(co_tcp_send_async_data_t), NULL);
    }

    co_tcp_send_async_data_t send_data = { 0 };

    send_data.data = data;
    send_data.data_size = data_size;
    send_data.user_data = user_data;

    co_queue_push(client->send_async_queue, &send_data);

#ifdef CO_OS_WIN

    ssize_t result =
        co_win_net_send_async(
            &client->sock, data, data_size);

    if (result == 0)
    {
        co_tcp_log_debug(
            &client->sock.local.net_addr,
            "-->",
            &client->sock.remote.net_addr,
            "tcp send async QUEUED %d bytes", data_size);

        return true;
    }
    else if (result > 0)
    {
        return true;
    }

#else

    if (co_queue_get_count(client->send_async_queue) > 1)
    {
        co_tcp_log_debug(
            &client->sock.local.net_addr,
            "-->",
            &client->sock.remote.net_addr,
            "tcp send async QUEUED %d bytes", data_size);

        return true;
    }

    if (co_socket_handle_send(
        client->sock.handle, data, data_size, 0) == (ssize_t)data_size)
    {
        co_thread_send_event(
            client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_SEND_ASYNC_COMPLETE,
            (uintptr_t)client,
            (uintptr_t)data_size);

        return true;
    }
    else
    {
        int error_code = co_socket_get_error();

        if ((error_code == EAGAIN) || (error_code == EWOULDBLOCK))
        {
            co_net_worker_t* net_worker =
                co_socket_get_net_worker(&client->sock);

            co_net_worker_set_tcp_send(net_worker, client, true);

            co_tcp_log_debug(
                &client->sock.local.net_addr,
                "-->",
                &client->sock.remote.net_addr,
                "tcp send async QUEUED %d bytes", data_size);

            return true;
        }
    }

#endif

    co_queue_remove(client->send_async_queue, 1);

    return false;
}

ssize_t
co_tcp_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size
)
{
#ifdef CO_OS_WIN
    ssize_t data_size =
        co_win_net_receive(
            &client->sock, buffer, buffer_size);
#else
    ssize_t data_size =
        co_socket_handle_receive(
            client->sock.handle, buffer, buffer_size, 0);
#endif

    if (data_size > 0)
    {
        co_tcp_log_debug_hex_dump(
            &client->sock.local.net_addr,
            "<--",
            &client->sock.remote.net_addr,
            buffer, data_size,
            "tcp receive %zd bytes", data_size);
    }
    else if (data_size == 0)
    {
        co_thread_send_event(
            client->sock.owner_thread,
            CO_NET_EVENT_ID_TCP_CLOSE,
            (uintptr_t)client,
            0);
    }

    return data_size;
}

ssize_t
co_tcp_receive_all(
    co_tcp_client_t* client,
    co_byte_array_t* byte_array
)
{
    ssize_t total = 0;

    for (;;)
    {
        char buffer[8192];

        ssize_t size =
            co_tcp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        co_byte_array_add(byte_array, buffer, size);

        total += size;
    }

    return total;
}

co_tcp_callbacks_st*
co_tcp_get_callbacks(
    co_tcp_client_t* client
)
{
    return &client->callbacks;
}

bool
co_tcp_half_close(
    co_tcp_client_t* client,
    uint32_t timeout_msec
)
{
    if (client == NULL)
    {
        return false;
    }

    return co_net_worker_close_tcp_client_local(
        co_socket_get_net_worker(&client->sock),
        client, timeout_msec);
}

void
co_tcp_close(
    co_tcp_client_t* client
)
{
    if (client == NULL)
    {
        return;
    }

    if (client->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    if (client->sock.owner_thread != NULL)
    {
        co_net_worker_unregister_tcp_connection(
            co_socket_get_net_worker(&client->sock), client);
    }

    client->callbacks.on_connect = NULL;
    client->callbacks.on_send_async = NULL;
    client->callbacks.on_receive = NULL;
    client->callbacks.on_receive_timer = NULL;
    client->callbacks.on_close = NULL;

    co_socket_handle_close(client->sock.handle);
    client->sock.handle = CO_SOCKET_INVALID_HANDLE;

    client->sock.local.is_open = false;
    client->sock.remote.is_open = false;

    co_net_addr_remove_unix_path_file(
        &client->sock.local.net_addr);
}

const co_net_addr_t*
co_tcp_get_remote_net_addr(
    const co_tcp_client_t* client
)
{
    return &client->sock.remote.net_addr;
}

bool
co_tcp_create_receive_timer(
    co_tcp_client_t* client,
    uint32_t msec
)
{
    if (co_socket_get_receive_timer(&client->sock) != NULL)
    {
        return false;
    }

    co_timer_t* receive_timer =
        co_timer_create(msec, co_tcp_client_on_receive_timer,
            false, client);

    if (receive_timer == NULL)
    {
        return false;
    }

    co_socket_set_receive_timer(
        &client->sock, receive_timer);

    return true;
}

void
co_tcp_destroy_receive_timer(
    co_tcp_client_t* client
)
{
    if (client != NULL)
    {
        co_timer_destroy(
            co_socket_get_receive_timer(
                &client->sock));

        co_socket_set_receive_timer(
            &client->sock, NULL);
    }
}

bool
co_tcp_start_receive_timer(
    co_tcp_client_t* client
)
{
    co_timer_t* receive_timer =
        co_socket_get_receive_timer(&client->sock);

    if (receive_timer == NULL)
    {
        return false;
    }

    return co_timer_start(receive_timer);
}

void
co_tcp_stop_receive_timer(
    co_tcp_client_t* client
)
{
    if (client == NULL)
    {
        return;
    }

    co_timer_t* receive_timer =
        co_socket_get_receive_timer(
            &client->sock);

    if (receive_timer == NULL)
    {
        return;
    }

    co_timer_stop(receive_timer);
}

bool
co_tcp_restart_receive_timer(
    co_tcp_client_t* client
)
{
    co_timer_t* receive_timer =
        co_socket_get_receive_timer(&client->sock);

    if (receive_timer == NULL)
    {
        return false;
    }

    co_timer_stop(receive_timer);

    return co_timer_start(receive_timer);
}

bool
co_tcp_is_running_receive_timer(
    const co_tcp_client_t* client
)
{
    if (client == NULL)
    {
        return false;
    }

    co_timer_t* receive_timer =
        co_socket_get_receive_timer(
            &client->sock);

    if (receive_timer == NULL)
    {
        return false;
    }

    return co_timer_is_running(receive_timer);
}

bool
co_tcp_is_open(
    const co_tcp_client_t* client
)
{
    return (client != NULL &&
        client->sock.local.is_open && client->sock.remote.is_open);
}

co_socket_t*
co_tcp_client_get_socket(
    co_tcp_client_t* client
)
{
    return &client->sock;
}

void
co_tcp_set_user_data(
    co_tcp_client_t* client,
    void* user_data
)
{
    if (client != NULL)
    {
        client->sock.user_data = user_data;
    }
}

void*
co_tcp_get_user_data(
    const co_tcp_client_t* client
)
{
    if (client != NULL)
    {
        return client->sock.user_data;
    }

    return NULL;
}
