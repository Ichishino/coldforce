#include <coldforce/core/co_std.h>

#include <coldforce/net/co_socket.h>

//---------------------------------------------------------------------------//
// socket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
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
    return &sock->local_net_addr;
}
