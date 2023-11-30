#ifndef CO_UDP_SERVER_H_INCLUDED
#define CO_UDP_SERVER_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_udp.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// udp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_udp_server_t;

typedef void(*co_udp_accept_fn)(
    co_thread_t* self, struct co_udp_server_t* udp_server, co_udp_t* udp_conn);

typedef struct
{
    co_udp_accept_fn on_accept;

} co_udp_server_callbacks_st;

typedef struct co_udp_server_t
{
    co_udp_t udp;
    co_udp_server_callbacks_st callbacks;

} co_udp_server_t;

typedef struct co_udp_connection_t
{
    co_udp_t udp;
    co_buffer_st accept_data;

} co_udp_connection_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_NET_API
co_udp_t*
co_udp_create_connection(
    const co_udp_server_t* udp_server,
    const co_net_addr_t* remote_net_addr,
    const uint8_t* data,
    size_t data_size
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
co_udp_server_t*
co_udp_server_create(
    const co_net_addr_t* local_net_addr
);

CO_NET_API
void
co_udp_server_destroy(
    co_udp_server_t* udp_server
);

CO_NET_API
bool
co_udp_server_start(
    co_udp_server_t* udp_server
);

CO_NET_API
bool
co_udp_accept(
    co_thread_t* owner_thread,
    co_udp_t* udp_conn
);

CO_NET_API
size_t
co_udp_get_accept_data(
    const co_udp_t* udp_conn,
    const uint8_t** buffer
);

CO_NET_API
void
co_udp_destroy_connection(
    co_udp_t* udp_conn
);

CO_NET_API
co_udp_server_callbacks_st*
co_udp_server_get_callbacks(
    co_udp_server_t* udp_server
);

CO_NET_API
co_socket_t*
co_udp_server_get_socket(
    co_udp_server_t* udp_server
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_UDP_SERVER_H_INCLUDED
