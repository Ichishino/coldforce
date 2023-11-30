#ifndef CO_DTLS_UDP_CLIENT_H_INCLUDED
#define CO_DTLS_UDP_CLIENT_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_udp.h>

#include <coldforce/tls/co_tls.h>
#include <coldforce/tls/co_tls_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// dtls udp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef co_tls_handshake_fn co_dtls_udp_handshake_fn;
typedef co_tls_callbacks_st co_dtls_udp_callbacks_st;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_TLS_API
co_udp_t*
co_dtls_udp_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_TLS_API
void
co_dtls_udp_client_destroy(
    co_udp_t* udp
);

CO_TLS_API
bool
co_dtls_udp_start_handshake(
    co_udp_t* udp,
    const co_net_addr_t* remote_net_addr
);

CO_TLS_API
bool
co_dtls_udp_send(
    co_udp_t* udp,
    const void* data,
    size_t data_size
);

CO_TLS_API
bool
co_dtls_udp_send_async(
    co_udp_t* udp,
    const void* data,
    size_t data_size,
    void* user_data
);

CO_TLS_API
ssize_t
co_dtls_udp_receive(
    co_udp_t* udp,
    void* buffer,
    size_t buffer_size
);

CO_TLS_API
ssize_t
co_dtls_udp_receive_all(
    co_udp_t* udp,
    co_byte_array_t* byte_array
);

CO_TLS_API
co_dtls_udp_callbacks_st*
co_dtls_udp_get_callbacks(
    co_udp_t* udp
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_DTLS_UDP_CLIENT_H_INCLUDED
