#ifndef CO_TCP_SERVER_H_INCLUDED
#define CO_TCP_SERVER_H_INCLUDED

#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_socket.h>
#include <coldforce/net/co_tcp_client.h>

#ifdef CO_OS_WIN
#include <coldforce/net/co_tcp_win.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_tcp_server_t;

typedef bool(*co_tcp_accept_fn)(
    void* self, struct co_tcp_server_t* server, co_tcp_client_t* client);

typedef void(*co_tcp_handover_fn)(
    void* self, co_tcp_client_t* client);

typedef struct co_tcp_server_t
{
    co_socket_t sock;

    co_tcp_accept_fn on_accept_ready;

#ifdef CO_OS_WIN
    co_win_tcp_server_extention_t win;
#endif

} co_tcp_server_t;

void co_tcp_server_on_accept_ready(co_tcp_server_t* server);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API co_tcp_server_t* co_tcp_server_create(const co_net_addr_t* local_net_addr);

CO_NET_API void co_tcp_server_destroy(co_tcp_server_t* server);
CO_NET_API void co_tcp_server_close(co_tcp_server_t* server);

CO_NET_API bool co_tcp_server_start(
    co_tcp_server_t* server, co_tcp_accept_fn handler, int backlog);

CO_NET_API bool co_tcp_accept(
    co_thread_t* owner_thread, co_tcp_client_t* client);

CO_NET_API void co_tcp_set_handover_handler(
    co_thread_t* thread, co_tcp_handover_fn handler);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TCP_SERVER_H_INCLUDED
