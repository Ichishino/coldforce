#ifndef CO_TLS_TCP_CLIENT_H_INCLUDED
#define CO_TLS_TCP_CLIENT_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/tls/co_tls.h>
#include <coldforce/tls/co_tls_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls tcp client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_TLS_API
co_tcp_client_t*
co_tls_tcp_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_TLS_API
void
co_tls_tcp_client_destroy(
    co_tcp_client_t* tcp_client
);

CO_TLS_API
bool
co_tls_tcp_start_handshake(
    co_tcp_client_t* tcp_client
);

CO_TLS_API
void
co_tls_tcp_set_host_name(
    co_tcp_client_t* tcp_client,
    const char* host_name
);

CO_TLS_API
void
co_tls_tcp_set_available_protocols(
    co_tcp_client_t* tcp_client,
    const char* protocols[],
    size_t count
);

CO_TLS_API
bool
co_tls_tcp_get_selected_protocol(
    const co_tcp_client_t* tcp_client,
    char* buffer,
    size_t buffer_size
);

CO_TLS_API
bool
co_tls_tcp_send(
    co_tcp_client_t* tcp_client,
    const void* data,
    size_t data_size
);

CO_TLS_API
bool
co_tls_tcp_send_async(
    co_tcp_client_t* tcp_client,
    const void* data,
    size_t data_size,
    void* user_data
);

CO_TLS_API
ssize_t
co_tls_tcp_receive(
    co_tcp_client_t* tcp_client,
    void* buffer,
    size_t buffer_size
);

CO_TLS_API
ssize_t
co_tls_tcp_receive_all(
    co_tcp_client_t* tcp_client,
    co_byte_array_t* byte_array
);

CO_TLS_API
co_tls_callbacks_st*
co_tls_tcp_get_callbacks(
    co_tcp_client_t* tcp_client
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_TCP_CLIENT_H_INCLUDED
