#include <coldforce/core/co_std.h>

#include <coldforce/net/co_udp.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_log.h>

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

void
co_udp_on_send_ready(
    co_udp_t* udp
)
{
    co_udp_log_debug(
        &udp->sock.local_net_addr,
        NULL,
        NULL,
        "udp send ready");

#ifdef CO_OS_WIN
    (void)udp;
#else

    co_udp_send_data_t* send_data =
        (co_udp_send_data_t*)co_queue_peek_head(udp->send_queue);

    if (send_data == NULL)
    {
        udp->sock_event_flags &= ~CO_SOCKET_EVENT_SEND;

        co_net_worker_update_udp(
            co_socket_get_net_worker(&udp->sock),
            udp);

        return;
    }

    ssize_t sent_size =
        co_socket_handle_send_to(
            udp->sock.handle,
            &send_data->remote_net_addr,
            send_data->buffer.ptr, send_data->buffer.size, 0);

    if ((sent_size > 0) && ((size_t)sent_size == send_data->buffer.size))
    {
        co_udp_send_data_t used_data;
        co_queue_pop(udp->send_queue, &used_data);

        co_mem_free(used_data.buffer.ptr);

        co_thread_send_event(
            udp->sock.owner_thread,
            CO_NET_EVENT_ID_UDP_SEND_COMPLETE,
            (uintptr_t)udp,
            (uintptr_t)sent_size);

        co_udp_on_send_ready(udp);
    }

#endif
}

void
co_udp_on_send_complete(
    co_udp_t* udp,
    size_t data_size
)
{
    co_udp_log_debug(
        &udp->sock.local_net_addr,
        NULL,
        NULL,
        "udp send complete");

#ifdef CO_OS_WIN
    co_list_remove_head(udp->win.io_send_ctxs);
#endif

    if (udp->callbacks.on_send_async != NULL)
    {
        udp->callbacks.on_send_async(
            udp->sock.owner_thread, udp, (data_size > 0));
    }
}

void
co_udp_on_receive_ready(
    co_udp_t* udp,
    size_t data_size
)
{
    co_udp_log_debug(
        &udp->sock.local_net_addr,
        NULL,
        NULL,
        "udp receive ready");

#ifdef CO_OS_WIN
    udp->win.receive.size = data_size;
#else
    (void)data_size;
#endif

    if (udp->callbacks.on_receive != NULL)
    {
        udp->callbacks.on_receive(udp->sock.owner_thread, udp);
    }

#ifdef CO_OS_WIN
    co_win_udp_receive_start(udp);
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

    co_socket_setup(&udp->sock);

    udp->sock.type = CO_SOCKET_TYPE_UDP;
    udp->sock.owner_thread = co_thread_get_current();
    udp->sock.open_local = true;

    memcpy(&udp->sock.local_net_addr,
        local_net_addr, sizeof(co_net_addr_t));

    udp->bound_local_net_addr = false;
    udp->sock_event_flags = 0;
    udp->callbacks.on_send_async = NULL;
    udp->callbacks.on_receive = NULL;

#ifdef CO_OS_WIN

    if (!co_win_udp_setup(
        udp, CO_WIN_UDP_DEFAULT_RECEIVE_BUFFER_SIZE))
    {
        co_mem_free(udp);

        return NULL;
    }

    if (!co_net_worker_register_udp(
        co_socket_get_net_worker(&udp->sock), udp))
    {
        co_win_udp_cleanup(udp);
        co_mem_free(udp);

        return NULL;
    }

#else

    udp->sock.handle = co_socket_handle_create(
        local_net_addr->sa.any.ss_family, SOCK_DGRAM, IPPROTO_UDP);

    if (udp->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        co_mem_free(udp);

        return NULL;
    }

    udp->send_queue = NULL;

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

#ifdef CO_OS_WIN
        co_win_udp_cleanup(udp);
#else
        if (udp->send_queue != NULL)
        {
            co_udp_send_data_t send_data;

            while (co_queue_pop(udp->send_queue, &send_data))
            {
                co_mem_free(send_data.buffer.ptr);
            }

            co_queue_destroy(udp->send_queue);
            udp->send_queue = NULL;
        }
#endif
        co_socket_cleanup(&udp->sock);
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

    if (udp->sock.open_local)
    {
        co_net_worker_unregister_udp(
            co_socket_get_net_worker(&udp->sock), udp);

        co_socket_handle_close(udp->sock.handle);
        udp->sock.handle = CO_SOCKET_INVALID_HANDLE;

        udp->sock.open_local = false;

        udp->callbacks.on_send_async = NULL;
        udp->callbacks.on_receive = NULL;
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
            &udp->sock.local_net_addr))
        {
            return false;
        }

        udp->bound_local_net_addr = true;
    }

    return true;
}

bool
co_udp_send(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
)
{
    co_udp_log_debug_hex_dump(
        &udp->sock.local_net_addr,
        "-->",
        remote_net_addr,
        data, data_size,
        "udp send %d bytes", data_size);

#ifdef CO_OS_WIN

    return co_win_udp_send(udp, remote_net_addr, data, data_size);

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
co_udp_send_async(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
)
{
    co_udp_log_debug_hex_dump(
        &udp->sock.local_net_addr,
        "-->",
        remote_net_addr,
        data, data_size,
        "udp send async %d bytes", data_size);

#ifdef CO_OS_WIN

    return co_win_udp_send_async(
        udp, remote_net_addr, data, data_size);

#else

    co_net_worker_t* net_worker =
        co_socket_get_net_worker(&udp->sock);

    udp->sock_event_flags |= CO_SOCKET_EVENT_SEND;
    co_net_worker_update_udp(net_worker, udp);

    if (co_socket_handle_send_to(udp->sock.handle,
        remote_net_addr, data, data_size, 0) == (ssize_t)data_size)
    {

        udp->sock_event_flags &= ~CO_SOCKET_EVENT_SEND;
        co_net_worker_update_udp(net_worker, udp);

        co_thread_send_event(
            udp->sock.owner_thread,
            CO_NET_EVENT_ID_UDP_SEND_COMPLETE,
            (uintptr_t)udp,
            data_size);

        return true;
    }
    else
    {
        int error_code = co_socket_get_error();

        if ((error_code == EAGAIN) || (error_code == EWOULDBLOCK))
        {
            if (udp->send_queue == NULL)
            {
                udp->send_queue = co_queue_create(
                    sizeof(co_udp_send_data_t), NULL);
            }

            co_udp_send_data_t send_data;

            memcpy(&send_data.remote_net_addr,
                remote_net_addr, sizeof(co_net_addr_t));

            send_data.buffer.size = data_size;
            send_data.buffer.ptr = co_mem_alloc(send_data.buffer.size);

            if (send_data.buffer.ptr != NULL)
            {
                memcpy(send_data.buffer.ptr, data, send_data.buffer.size);
                co_queue_push(udp->send_queue, &send_data);

                return true;
            }
        }
    }

    udp->sock_event_flags &= ~CO_SOCKET_EVENT_SEND;
    co_net_worker_update_udp(net_worker, udp);

    return false;

#endif
}

bool
co_udp_receive_start(
    co_udp_t* udp
)
{
    if (udp->sock_event_flags & CO_SOCKET_EVENT_RECEIVE)
    {
        return true;
    }

    if (!co_udp_bind_local_net_addr(udp))
    {
        return false;
    }

    co_udp_log_info(
        &udp->sock.local_net_addr,
        NULL,
        NULL,
        "udp receive start");

    udp->sock_event_flags |= CO_SOCKET_EVENT_RECEIVE;

#ifdef CO_OS_WIN

    if (!co_win_udp_receive_start(udp))
    {
        return false;
    }

#else

    if (!co_net_worker_update_udp(
        co_socket_get_net_worker(&udp->sock), udp))
    {
        return false;
    }

#endif

    return true;
}

ssize_t
co_udp_receive(
    co_udp_t* udp,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size
)
{
    ssize_t result = -1;

#ifdef CO_OS_WIN

    size_t data_size = 0;

    if (co_win_udp_receive(
        udp, remote_net_addr,
        buffer, buffer_size, &data_size))
    {
        result = (ssize_t)data_size;
    }

#else

    result = co_socket_handle_receive_from(
        udp->sock.handle, remote_net_addr, buffer, buffer_size, 0);

#endif

    if (result > 0)
    {
        co_udp_log_debug_hex_dump(
            &udp->sock.local_net_addr,
            "<--",
            remote_net_addr,
            buffer, result,
            "udp receive %d bytes", result);
    }

    return result;
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
