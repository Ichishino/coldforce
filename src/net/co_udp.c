#include <coldforce/core/co_std.h>

#include <coldforce/net/co_udp.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// udp
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_udp_on_send(
    co_udp_t* udp,
    size_t data_length
)
{
    (void)data_length;

#ifdef CO_OS_WIN
    co_list_remove_head(udp->win.io_send_ctxs);
#endif
}

void
co_udp_on_receive(
    co_udp_t* udp,
    size_t data_length
)
{
#ifdef CO_OS_WIN
    udp->win.receive.length = data_length;
#endif

    co_assert(udp->on_receive != NULL);

    udp->on_receive(udp->sock.owner_thread, udp);

#ifdef CO_OS_WIN
    co_win_udp_receive_start(udp);
#endif
}

//---------------------------------------------------------------------------//
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

    udp->sock.owner_thread = co_thread_get_current();
    udp->sock.open_local = true;

    memcpy(&udp->sock.local_net_addr,
        local_net_addr, sizeof(co_net_addr_t));

    udp->on_receive = NULL;
    udp->bound_local_net_addr = false;

#ifdef CO_OS_WIN
    if (!co_win_udp_setup(
        udp, CO_WIN_UDP_DEFAULT_RECEIVE_BUFF_LENGTH))
    {
        co_mem_free(udp);

        return NULL;
    }
#else

#endif

    if (!co_net_worker_register_udp(
        (co_net_worker_t*)udp->sock.owner_thread->event_worker,
        udp))
    {
#ifdef CO_OS_WIN
        co_win_udp_cleanup(udp);
#endif
        co_mem_free(udp);

        return false;
    }

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
#endif
        co_mem_free(udp);
    }
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
            (co_net_worker_t*)udp->sock.owner_thread->event_worker,
            udp);

        co_socket_handle_close(udp->sock.handle);
        udp->sock.handle = CO_SOCKET_INVALID_HANDLE;

        udp->sock.open_local = false;
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

void
co_udp_send(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_length
)
{
#ifdef CO_OS_WIN
    co_win_udp_send_async(
        udp, remote_net_addr, data, data_length);
#else
#endif
}

void
co_udp_send_string(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr,
    const char* data
)
{
    co_udp_send(
        udp, remote_net_addr, data, strlen(data) + 1);
}

bool
co_udp_receive_start(
    co_udp_t* udp,
    co_udp_receive_fn handler
)
{
    if (!co_udp_bind_local_net_addr(udp))
    {
        return false;
    }

#ifdef CO_OS_WIN
    if (!co_win_udp_receive_start(udp))
    {
        return false;
    }
#endif

    udp->on_receive = handler;

    return true;
}

ssize_t
co_udp_receive(
    co_udp_t* udp,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_length
)
{
#ifdef CO_OS_WIN

    size_t received_length = 0;

    if (!co_win_udp_receive(
        udp, remote_net_addr,
        buffer, buffer_length, &received_length))
    {
        return -1;
    }
    else
    {
        return (int)received_length;
    }

#else
    return 0;
#endif
}
