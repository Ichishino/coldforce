#include <coldforce/core/co_std.h>

#include <coldforce/net/co_socket.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// socket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_socket_setup(
    co_socket_t* sock,
    co_socket_type_t type
)
{
    sock->type = type;
    sock->owner_thread = NULL;
    sock->handle = CO_SOCKET_INVALID_HANDLE;

    co_net_addr_init(&sock->local.net_addr);
    co_net_addr_init(&sock->remote.net_addr);
    sock->local.is_open = false;
    sock->remote.is_open = false;

    sock->receive_timer = NULL;

    sock->sub_class = NULL;
    sock->tls = NULL;
    sock->user_data = NULL;
}

void
co_socket_cleanup(
    co_socket_t* sock
)
{
    co_socket_handle_close(sock->handle);
    sock->handle = CO_SOCKET_INVALID_HANDLE;

    sock->type = CO_SOCKET_TYPE_UNKNOWN;
    sock->owner_thread = NULL;

    co_net_addr_init(&sock->local.net_addr);
    co_net_addr_init(&sock->remote.net_addr);
    sock->local.is_open = false;
    sock->remote.is_open = false;

    co_timer_destroy(sock->receive_timer);
    sock->receive_timer = NULL;

    sock->sub_class = NULL;
    sock->tls = NULL;
    sock->user_data = 0;
}

co_net_worker_t*
co_socket_get_net_worker(
    co_socket_t* sock
)
{
    return (co_net_worker_t*)sock->owner_thread->event_worker;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_socket_handle_t
co_socket_get_handle(
    const co_socket_t* sock
)
{
    return sock->handle;
}

co_thread_t*
co_socket_get_owner_thread(
    const co_socket_t* sock
)
{
    return sock->owner_thread;
}

const co_net_addr_t*
co_socket_get_local_net_addr(
    const co_socket_t* sock
)
{
    return &sock->local.net_addr;
}

const co_net_addr_t*
co_socket_get_remote_net_addr(
    const co_socket_t* sock
)
{
    return &sock->remote.net_addr;
}

void
co_socket_set_receive_timer(
    co_socket_t* sock,
    co_timer_t* timer
)
{
    sock->receive_timer = timer;
}

co_timer_t*
co_socket_get_receive_timer(
    const co_socket_t* sock
)
{
    return sock->receive_timer;
}
