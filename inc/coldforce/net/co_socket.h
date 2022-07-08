#ifndef CO_SOCKET_H_INCLUDED
#define CO_SOCKET_H_INCLUDED

#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket_handle.h>

CO_EXTERN_C_BEGIN

struct co_net_worker_t;

//---------------------------------------------------------------------------//
// socket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef enum
{
    CO_SOCKET_TYPE_TCP_SERVER           = 1,
    CO_SOCKET_TYPE_TCP_CONNECTOR,
    CO_SOCKET_TYPE_TCP_CONNECTION,
    CO_SOCKET_TYPE_UDP

} co_socket_type_t;

typedef struct
{
    co_socket_handle_t handle;

    co_thread_t* owner_thread;
    co_net_addr_t local_net_addr;
    co_socket_type_t type;

    bool open_local;
    void* sub_class;
    void* tls;

    void* user_data;

} co_socket_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_socket_setup(
    co_socket_t* sock
);

void
co_socket_cleanup(
    co_socket_t* sock
);

struct co_net_worker_t*
co_socket_get_net_worker(
    co_socket_t* sock
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
co_socket_handle_t
co_socket_get_handle(
    const co_socket_t* sock
);

CO_NET_API
co_thread_t*
co_socket_get_owner_thread(
    const co_socket_t* sock
);

CO_NET_API
const co_net_addr_t*
co_socket_get_local_net_addr(
    const co_socket_t* sock
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SOCKET_H_INCLUDED
