#ifndef CO_TLS_TCP_SERVER_H_INCLUDED
#define CO_TLS_TCP_SERVER_H_INCLUDED

#include <coldforce/net/co_tcp_server.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_tls_ctx_st ctx;

    co_tcp_accept_fn on_accept_ready;

} co_tls_tcp_server_t;

#define co_tcp_server_get_tls(server) \
    ((co_tls_tcp_server_t*)server->tls)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_TLS_API co_tcp_server_t* co_tls_tcp_server_create(
    const co_net_addr_t* local_net_addr, co_tls_ctx_st* tls_ctx);

CO_TLS_API void co_tls_tcp_server_destroy(co_tcp_server_t* server);
CO_TLS_API void co_tls_tcp_server_close(co_tcp_server_t* server);

CO_TLS_API bool co_tls_tcp_server_start(
    co_tcp_server_t* server, co_tcp_accept_fn handler, int backlog);

CO_TLS_API bool co_tls_tcp_accept(
    co_thread_t* owner_thread, co_tcp_client_t* client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_TCP_SERVER_H_INCLUDED
