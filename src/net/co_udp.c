#include <coldforce/core/co_std.h>

#include <coldforce/net/co_udp.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_log.h>
#include <coldforce/net/co_socket_option.h>

#ifndef CO_OS_WIN
#include <errno.h>
#endif

//---------------------------------------------------------------------------//
// udp
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static bool
co_udp_setup(
    co_udp_t* udp,
    co_socket_type_t type
)
{
    co_socket_setup(&udp->sock, type);

    udp->bound_local_net_addr = false;
    udp->send_async_queue = NULL;
    udp->callbacks.on_send_async = NULL;
    udp->callbacks.on_receive = NULL;

#ifdef CO_OS_WIN
    if (!co_win_net_client_extension_setup(
        &udp->sock.win.client, &udp->sock,
        CO_WIN_UDP_DEFAULT_RECEIVE_BUFFER_SIZE))
    {
        return false;
    }

    udp->sock.win.client.receive.remote_net_addr =
        (co_net_addr_t*)co_mem_alloc(sizeof(co_net_addr_t));

    if (udp->sock.win.client.receive.remote_net_addr == NULL)
    {
        co_win_net_client_extension_cleanup(
            &udp->sock.win.client);

        return false;
    }
#else
    udp->sock_event_flags = 0;
#endif

    return true;
}

static void
co_udp_cleanup(
    co_udp_t* udp
)
{
#ifdef CO_OS_WIN
    co_win_net_client_extension_cleanup(
        &udp->sock.win.client);
#endif
    if (udp->send_async_queue != NULL)
    {
        co_queue_destroy(udp->send_async_queue);
        udp->send_async_queue = NULL;
    }

    udp->callbacks.on_send_async = NULL;
    udp->callbacks.on_receive = NULL;

    co_socket_cleanup(&udp->sock);
}

#ifndef CO_OS_WIN
void
co_udp_on_send_async_ready(
    co_udp_t* udp
)
{
    if (udp->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_udp_send_async_data_t* send_data =
        (co_udp_send_async_data_t*)co_queue_peek_head(udp->send_async_queue);

    if (send_data == NULL)
    {
        udp->sock_event_flags &= ~CO_SOCKET_EVENT_SEND;

        co_net_worker_update_udp(
            co_socket_get_net_worker(&udp->sock),
            udp);

        return;
    }

    co_udp_log_debug(
        &udp->sock.local.net_addr,
        NULL,
        NULL,
        "udp send(to) async ready");

    ssize_t sent_size;

    if (send_data->remote_net_addr != NULL)
    {
        sent_size = co_socket_handle_send_to(
            udp->sock.handle,
            send_data->remote_net_addr,
            send_data->data, send_data->data_size, 0);
    }
    else
    {
        sent_size = co_socket_handle_send(
            udp->sock.handle,
            send_data->data, send_data->data_size, 0);
    }

    if ((size_t)sent_size == send_data->data_size)
    {
        co_udp_on_send_async_complete(udp, true);
        co_udp_on_send_async_ready(udp);
    }
    else
    {
        co_udp_log_debug(
            &udp->sock.local.net_addr,
            NULL,
            NULL,
            "udp send(to) async QUEUED %zd items",
            co_queue_get_count(udp->send_async_queue));
    }
}
#endif // !CO_OS_WIN

void
co_udp_on_send_async_complete(
    co_udp_t* udp,
    bool result
)
{
    if (udp->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_udp_send_async_data_t send_data;

    if ((udp->send_async_queue == NULL) ||
        !co_queue_pop(udp->send_async_queue, &send_data))
    {
        return;
    }

    co_udp_log_debug_hex_dump(
        &udp->sock.local.net_addr,
        "-->",
        (send_data.remote_net_addr != NULL) ?
        send_data.remote_net_addr : &udp->sock.remote.net_addr,
        send_data.data, send_data.data_size,
        "udp send(to) async %d bytes", send_data.data_size);

    if (udp->callbacks.on_send_async != NULL)
    {
        udp->callbacks.on_send_async(
            udp->sock.owner_thread, udp,
            send_data.user_data, result);
    }
}

void
co_udp_on_receive_ready(
    co_udp_t* udp,
    size_t data_size
)
{
    if (udp->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return;
    }

    co_udp_log_debug(
        &udp->sock.local.net_addr,
        NULL,
        NULL,
        "udp receive ready");

#ifdef CO_OS_WIN
    udp->sock.win.client.receive.size = data_size;
#else
    (void)data_size;
#endif

    if (udp->callbacks.on_receive != NULL)
    {
        udp->callbacks.on_receive(udp->sock.owner_thread, udp);
    }

#ifdef CO_OS_WIN
    if (udp->sock.type == CO_SOCKET_TYPE_UDP)
    {
        co_win_net_receive_from_start(&udp->sock);
    }
    else
    {
        co_win_net_receive_start(&udp->sock);
    }
#endif
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_udp_t*
co_udp_create(
    const co_net_addr_t* local_net_addr
)
{
    co_udp_t* udp =
        (co_udp_t*)co_mem_alloc(sizeof(co_udp_t));

    if (udp == NULL)
    {
        return NULL;
    }

    if (!co_udp_setup(
        udp, CO_SOCKET_TYPE_UDP))
    {
        co_udp_cleanup(udp);
        co_mem_free(udp);

        return NULL;
    }

    udp->sock.owner_thread = co_thread_get_current();
    udp->sock.local.is_open = true;

    memcpy(&udp->sock.local.net_addr,
        local_net_addr, sizeof(co_net_addr_t));

#ifdef CO_OS_WIN

    udp->sock.handle =
        co_win_socket_handle_create_udp(
            local_net_addr->sa.any.ss_family);

    if (udp->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        co_udp_cleanup(udp);
        co_mem_free(udp);

        return NULL;
    }

    if (!co_net_worker_register_udp(
        co_socket_get_net_worker(&udp->sock), udp))
    {
        co_udp_cleanup(udp);
        co_mem_free(udp);

        return NULL;
    }

#else

    udp->sock.handle =
        co_socket_handle_create(
            local_net_addr->sa.any.ss_family,
            SOCK_DGRAM, IPPROTO_UDP);

    if (udp->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        co_mem_free(udp);

        return NULL;
    }

#endif

    return udp;
}

void
co_udp_destroy(
    co_udp_t* udp
)
{
    if (udp != NULL)
    {
        co_udp_close(udp);
        co_udp_cleanup(udp);
        co_mem_free_later(udp);
    }
}

co_udp_callbacks_st*
co_udp_get_callbacks(
    co_udp_t* udp
)
{
    return &udp->callbacks;
}

void
co_udp_close(
    co_udp_t* udp
)
{
    if (udp == NULL)
    {
        return;
    }

    if (udp->sock.local.is_open)
    {
        co_net_worker_unregister_udp(
            co_socket_get_net_worker(&udp->sock), udp);

        co_socket_handle_close(udp->sock.handle);
        udp->sock.handle = CO_SOCKET_INVALID_HANDLE;

        udp->sock.local.is_open = false;
    }
}

bool
co_udp_bind_local_net_addr(
    co_udp_t* udp
)
{
    if (!udp->bound_local_net_addr)
    {
        if (!co_socket_handle_bind(
            udp->sock.handle,
            &udp->sock.local.net_addr))
        {
            return false;
        }

        co_socket_handle_get_local_net_addr(
            udp->sock.handle, &udp->sock.local.net_addr);

        udp->bound_local_net_addr = true;
    }

    return true;
}

bool
co_udp_send_to(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
)
{
    co_udp_log_debug_hex_dump(
        &udp->sock.local.net_addr,
        "-->",
        remote_net_addr,
        data, data_size,
        "udp sendto %d bytes", data_size);

#ifdef CO_OS_WIN

    return co_win_net_send_to(
        &udp->sock, remote_net_addr, data, data_size);

#else

    co_socket_handle_set_blocking(udp->sock.handle, true);

    ssize_t sent_size =
        co_socket_handle_send_to(
            udp->sock.handle, remote_net_addr, data, data_size, 0);

    co_socket_handle_set_blocking(udp->sock.handle, false);

    return (data_size == (size_t)sent_size);

#endif
}

bool
co_udp_send_to_async(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size,
    void* user_data
)
{
    if (udp->send_async_queue == NULL)
    {
        udp->send_async_queue = co_queue_create(
            sizeof(co_udp_send_async_data_t), NULL);
    }

    co_udp_send_async_data_t send_data = { 0 };

    send_data.remote_net_addr = remote_net_addr;

    send_data.data = data;
    send_data.data_size = data_size;
    send_data.user_data = user_data;

    co_queue_push(udp->send_async_queue, &send_data);

#ifdef CO_OS_WIN

    ssize_t result =
        co_win_net_send_to_async(
            &udp->sock, remote_net_addr, data, data_size);

    if (result == 0)
    {
        co_udp_log_debug(
            &udp->sock.local.net_addr,
            "-->",
            remote_net_addr,
            "udp sendto async QUEUED %d bytes", data_size);

        return true;
    }
    else if (result > 0)
    {
        return true;
    }

#else

    if (co_queue_get_count(udp->send_async_queue) > 1)
    {
        co_udp_log_debug(
            &udp->sock.local.net_addr,
            "-->",
            remote_net_addr,
            "udp sendto async QUEUED %d bytes", data_size);

        return true;
    }

    if (co_socket_handle_send_to(udp->sock.handle,
        remote_net_addr, data, data_size, 0) == (ssize_t)data_size)
    {
        co_thread_send_event(
            udp->sock.owner_thread,
            CO_NET_EVENT_ID_UDP_SEND_ASYNC_COMPLETE,
            (uintptr_t)udp,
            (uintptr_t)data_size);

        return true;
    }
    else
    {
        int error_code = co_socket_get_error();

        if ((error_code == EAGAIN) || (error_code == EWOULDBLOCK))
        {
            co_net_worker_t* net_worker =
                co_socket_get_net_worker(&udp->sock);

            udp->sock_event_flags |= CO_SOCKET_EVENT_SEND;
            co_net_worker_update_udp(net_worker, udp);

            co_udp_log_debug(
                &udp->sock.local.net_addr,
                "-->",
                remote_net_addr,
                "udp send async QUEUED %d bytes", data_size);

            return true;
        }
    }

#endif

    co_queue_remove(udp->send_async_queue, 1);

    return false;
}

bool
co_udp_receive_start(
    co_udp_t* udp
)
{
#ifndef CO_OS_WIN
    if (udp->sock_event_flags & CO_SOCKET_EVENT_RECEIVE)
    {
        return true;
    }
#endif

    if (!co_udp_bind_local_net_addr(udp))
    {
        return false;
    }

    co_udp_log_info(
        &udp->sock.local.net_addr,
        NULL,
        NULL,
        "udp receive start");

#ifdef CO_OS_WIN

    if (!co_win_net_receive_from_start(&udp->sock))
    {
        return false;
    }

#else

    udp->sock_event_flags |= CO_SOCKET_EVENT_RECEIVE;

    if (!co_net_worker_update_udp(
        co_socket_get_net_worker(&udp->sock), udp))
    {
        return false;
    }

#endif

    return true;
}

ssize_t
co_udp_receive_from(
    co_udp_t* udp,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size
)
{
#ifdef CO_OS_WIN
    ssize_t result =
        co_win_net_receive_from(
            &udp->sock, remote_net_addr,
            buffer, buffer_size);
#else
    ssize_t result =
        co_socket_handle_receive_from(
            udp->sock.handle, remote_net_addr,
            buffer, buffer_size, 0);
#endif

    if (result > 0)
    {
        co_udp_log_debug_hex_dump(
            &udp->sock.local.net_addr,
            "<--",
            remote_net_addr,
            buffer, result,
            "udp receivefrom %d bytes", result);
    }

    return result;
}

bool
co_udp_connect(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr
)
{
    if (udp->sock.type == CO_SOCKET_TYPE_UDP_CONNECTED)
    {
        return false;
    }

    if (!co_socket_handle_connect(
        udp->sock.handle, remote_net_addr))
    {
        return false;
    }

    co_socket_handle_get_local_net_addr(
        udp->sock.handle, &udp->sock.local.net_addr);
    co_socket_handle_get_remote_net_addr(
        udp->sock.handle, &udp->sock.remote.net_addr);

    co_udp_log_info(
        &udp->sock.local.net_addr,
        "-->",
        &udp->sock.remote.net_addr,
        "udp connect");

    udp->bound_local_net_addr = true;
    udp->sock.type = CO_SOCKET_TYPE_UDP_CONNECTED;

    return true;
}

bool
co_udp_send(
    co_udp_t* udp_conn,
    const void* data,
    size_t data_size
)
{
    if (udp_conn->sock.type != CO_SOCKET_TYPE_UDP_CONNECTED)
    {
        return false;
    }

    co_udp_log_debug_hex_dump(
        &udp_conn->sock.local.net_addr,
        "-->",
        &udp_conn->sock.remote.net_addr,
        data, data_size,
        "udp send %zd bytes", data_size);

#ifdef CO_OS_WIN

    return co_win_net_send(
        &udp_conn->sock, data, data_size);

#else

    co_socket_handle_set_blocking(udp_conn->sock.handle, true);

    ssize_t sent_size =
        co_socket_handle_send(
            udp_conn->sock.handle, data, data_size, 0);

    co_socket_handle_set_blocking(udp_conn->sock.handle, false);

    return (data_size == (size_t)sent_size);

#endif
}

bool
co_udp_send_async(
    co_udp_t* udp_conn,
    const void* data,
    size_t data_size,
    void* user_data
)
{
    if (udp_conn->sock.type != CO_SOCKET_TYPE_UDP_CONNECTED)
    {
        return false;
    }

    if (udp_conn->send_async_queue == NULL)
    {
        udp_conn->send_async_queue =
            co_queue_create(
                sizeof(co_udp_send_async_data_t), NULL);
    }

    co_udp_send_async_data_t send_data = { 0 };

    send_data.data = data;
    send_data.data_size = data_size;
    send_data.user_data = user_data;

    co_queue_push(udp_conn->send_async_queue, &send_data);

#ifdef CO_OS_WIN

    ssize_t result =
        co_win_net_send_async(
            &udp_conn->sock, data, data_size);

    if (result == 0)
    {
        co_udp_log_debug(
            &udp_conn->sock.local.net_addr,
            "-->",
            &udp_conn->sock.remote.net_addr,
            "udp send async QUEUED %d bytes", data_size);

        return true;
    }
    else if (result > 0)
    {
        return true;
    }

#else

    if (co_queue_get_count(udp_conn->send_async_queue) > 1)
    {
        co_udp_log_debug(
            &udp_conn->sock.local.net_addr,
            "-->",
            &udp_conn->sock.remote.net_addr,
            "udp send async QUEUED %d bytes", data_size);

        return true;
    }

    if (co_socket_handle_send(
        udp_conn->sock.handle, data, data_size, 0) == (ssize_t)data_size)
    {
        co_thread_send_event(
            udp_conn->sock.owner_thread,
            CO_NET_EVENT_ID_UDP_SEND_ASYNC_COMPLETE,
            (uintptr_t)udp_conn,
            (uintptr_t)data_size);

        return true;
    }
    else
    {
        int error_code = co_socket_get_error();

        if ((error_code == EAGAIN) || (error_code == EWOULDBLOCK))
        {
            co_net_worker_t* net_worker =
                co_socket_get_net_worker(&udp_conn->sock);

            udp_conn->sock_event_flags |= CO_SOCKET_EVENT_SEND;
            co_net_worker_update_udp(net_worker, udp_conn);

            co_udp_log_debug(
                &udp_conn->sock.local.net_addr,
                "-->",
                &udp_conn->sock.remote.net_addr,
                "udp send async QUEUED %d bytes", data_size);

            return true;
        }
    }

#endif

    co_queue_remove(udp_conn->send_async_queue, 1);

    return false;
}

ssize_t
co_udp_receive(
    co_udp_t* udp_conn,
    void* buffer,
    size_t buffer_size
)
{
    if (udp_conn->sock.type != CO_SOCKET_TYPE_UDP_CONNECTED)
    {
        return -1;
    }

#ifdef CO_OS_WIN
    ssize_t data_size =
        co_win_net_receive(
            &udp_conn->sock, buffer, buffer_size);
#else
    ssize_t data_size =
        co_socket_handle_receive(
            udp_conn->sock.handle, buffer, buffer_size, 0);
#endif

    if (data_size > 0)
    {
        co_udp_log_debug_hex_dump(
            &udp_conn->sock.local.net_addr,
            "<--",
            &udp_conn->sock.remote.net_addr,
            buffer, data_size,
            "udp receive %zd bytes", data_size);
    }

    return data_size;
}

co_udp_t*
co_udp_create_connection(
    const co_udp_t* udp,
    const co_net_addr_t* remote_net_addr
)
{
#ifdef CO_OS_WIN
    co_socket_handle_t handle =
        co_win_socket_handle_create_udp(
            udp->sock.local.net_addr.sa.any.ss_family);
#else
    co_socket_handle_t handle =
        co_socket_handle_create(
            udp->sock.local.net_addr.sa.any.ss_family,
            SOCK_DGRAM, IPPROTO_UDP);
#endif

    if (handle == CO_SOCKET_INVALID_HANDLE)
    {
        return NULL;
    }

    co_udp_t* udp_conn =
        (co_udp_t*)co_mem_alloc(sizeof(co_udp_t));

    if (udp_conn == NULL)
    {
        return NULL;
    }

    if (!co_udp_setup(
        udp_conn, CO_SOCKET_TYPE_UDP))
    {
        co_udp_cleanup(udp_conn);
        co_mem_free(udp_conn);

        return NULL;
    }

    udp_conn->sock.handle = handle;
    udp_conn->sock.local.is_open = true;

    memcpy(&udp_conn->sock.local.net_addr,
        &udp->sock.local.net_addr, sizeof(co_net_addr_t));
    memcpy(&udp_conn->sock.remote.net_addr,
        remote_net_addr, sizeof(co_net_addr_t));

    return udp_conn;
}

bool
co_udp_accept(
    co_thread_t* owner_thread,
    co_udp_t* udp_conn
)
{
    if (co_thread_get_current() != owner_thread)
    {
        CO_DEBUG_SOCKET_COUNTER_DEC();

        return co_thread_send_event(owner_thread,
            CO_NET_EVENT_ID_UDP_ACCEPT_ON_THREAD, (uintptr_t)udp_conn, 0);
    }

    co_udp_log_info(
        &udp_conn->sock.local.net_addr,
        "<--",
        &udp_conn->sock.remote.net_addr,
        "udp accept");

    udp_conn->sock.owner_thread = owner_thread;

#ifdef CO_OS_WIN
    if (!co_net_worker_register_udp(
        co_socket_get_net_worker(&udp_conn->sock), udp_conn))
    {
        return false;
    }
#endif

    co_socket_option_set_reuse_addr(&udp_conn->sock, true);

    if (!co_udp_bind_local_net_addr(udp_conn))
    {
        return false;
    }

    if (!co_udp_connect(
        udp_conn, &udp_conn->sock.remote.net_addr))
    {
        return false;
    }

    if (!co_udp_receive_start(udp_conn))
    {
        return false;
    }

    return true;
}

co_socket_t*
co_udp_get_socket(
    co_udp_t* udp
)
{
    return &udp->sock;
}

void
co_udp_set_user_data(
    co_udp_t* udp,
    void* user_data
)
{
    if (udp != NULL)
    {
        udp->sock.user_data = user_data;
    }
}

void*
co_udp_get_user_data(
    const co_udp_t* udp
)
{
    if (udp != NULL)
    {
        return udp->sock.user_data;
    }

    return NULL;
}
