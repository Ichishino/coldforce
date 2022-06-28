#ifndef CO_SOCKET_HANDLE_H_INCLUDED
#define CO_SOCKET_HANDLE_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// socket handle
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef CO_OS_WIN
typedef SOCKET co_socket_handle_t;
#define CO_SOCKET_INVALID_HANDLE    ((co_socket_handle_t)INVALID_SOCKET)
#else
typedef int co_socket_handle_t;
#define CO_SOCKET_INVALID_HANDLE    (-1)
#endif

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_NET_API
co_socket_handle_t
co_socket_handle_create(
    int family,
    int type,
    int protocol
);

CO_NET_API
void
co_socket_handle_close(
    co_socket_handle_t handle
);

CO_NET_API
bool
co_socket_handle_shutdown(
    co_socket_handle_t handle,
    int how
);

CO_NET_API
bool
co_socket_handle_bind(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr
);

CO_NET_API
bool
co_socket_handle_listen(
    co_socket_handle_t handle,
    int backlog
);

CO_NET_API
co_socket_handle_t
co_socket_handle_accept(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
);

CO_NET_API
bool
co_socket_handle_connect(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr
);

CO_NET_API
ssize_t
co_socket_handle_send(
    co_socket_handle_t handle,
    const void* data,
    size_t data_size,
    int flags
);

CO_NET_API
ssize_t
co_socket_handle_receive(
    co_socket_handle_t handle,
    void* buffer,
    size_t buffer_size,
    int flags
);

CO_NET_API
ssize_t
co_socket_handle_send_to(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr,
    const void* data,
    size_t data_size,
    int flags
);

CO_NET_API
ssize_t
co_socket_handle_receive_from(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr,
    void* buffer,
    size_t buffer_size,
    int flags
);

CO_NET_API
bool
co_socket_handle_set_option(
    co_socket_handle_t handle,
    int level,
    int name,
    const void* data,
    size_t data_size
);

CO_NET_API
bool
co_socket_handle_get_option(
    co_socket_handle_t handle,
    int level,
    int name,
    void* buffer,
    size_t* buffer_size
);

CO_NET_API
bool
co_socket_handle_get_local_net_addr(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
);

CO_NET_API
bool
co_socket_handle_get_remote_net_addr(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
);

CO_NET_API
bool
co_socket_handle_set_blocking(
    co_socket_handle_t handle,
    bool enable
);

CO_NET_API
int
co_socket_get_error(
    void
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SOCKET_HANDLE_H_INCLUDED
