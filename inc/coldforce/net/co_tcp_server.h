#ifndef CO_TCP_SERVER_H_INCLUDED
#define CO_TCP_SERVER_H_INCLUDED

#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_socket.h>
#include <coldforce/net/co_tcp_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_tcp_server_t;

typedef void(*co_tcp_accept_fn)(
    co_thread_t* self, struct co_tcp_server_t* server, co_tcp_client_t* client);

typedef struct
{
    co_tcp_accept_fn on_accept;

} co_tcp_server_callbacks_st;

typedef struct co_tcp_server_t
{
    co_socket_t sock;
    co_tcp_server_callbacks_st callbacks;

} co_tcp_server_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_tcp_server_on_accept_ready(
    co_tcp_server_t* server
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
co_tcp_server_t*
co_tcp_server_create(
    const co_net_addr_t* local_net_addr
);

CO_NET_API
void
co_tcp_server_destroy(
    co_tcp_server_t* server
);

CO_NET_API
co_tcp_server_callbacks_st*
co_tcp_server_get_callbacks(
    co_tcp_server_t* server
);

CO_NET_API
void
co_tcp_server_close(
    co_tcp_server_t* server
);

CO_NET_API
bool
co_tcp_server_start(
    co_tcp_server_t* server,
    int backlog
);

CO_NET_API
bool
co_tcp_accept(
    co_thread_t* owner_thread,
    co_tcp_client_t* client
);

CO_NET_API
co_socket_t*
co_tcp_server_get_socket(
    co_tcp_server_t* server
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TCP_SERVER_H_INCLUDED
