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

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_TLS_API
co_tcp_server_t*
co_tls_tcp_server_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_TLS_API
void
co_tls_tcp_server_destroy(
    co_tcp_server_t* tcp_server
);

CO_TLS_API
void
co_tls_tcp_server_set_available_protocols(
    co_tcp_server_t* tcp_server,
    const char* protocols[],
    size_t protocol_count
);

CO_TLS_API
bool
co_tls_tcp_server_start(
    co_tcp_server_t* tcp_server,
    int backlog
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_TCP_SERVER_H_INCLUDED
