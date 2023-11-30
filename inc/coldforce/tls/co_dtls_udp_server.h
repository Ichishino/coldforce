#ifndef CO_DTLS_UDP_SERVER_H_INCLUDED
#define CO_DTLS_UDP_SERVER_H_INCLUDED

#include <coldforce/net/co_udp_server.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// dtls udp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_TLS_API
co_udp_server_t*
co_dtls_udp_server_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_TLS_API
void
co_dtls_udp_server_destroy(
    co_udp_server_t* udp_server
);

CO_TLS_API
bool
co_dtls_udp_server_start(
    co_udp_server_t* udp_server
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_DTLS_UDP_SERVER_H_INCLUDED
