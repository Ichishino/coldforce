#ifndef CO_TLS_CLIENT_H_INCLUDED
#define CO_TLS_CLIENT_H_INCLUDED

#include <coldforce/core/co_queue.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef void(*co_tls_handshake_fn)(
    co_thread_t* self, co_tcp_client_t* client, int error_code);

typedef struct
{
    co_tls_handshake_fn on_handshake;

} co_tls_callbacks_st;

typedef struct
{
    co_tls_ctx_st ctx;
    co_tls_callbacks_st callbacks;

    co_tcp_receive_fn on_receive;

    co_byte_array_t* send_data;
    co_queue_t* receive_data_queue;

    CO_SSL_T* ssl;
    CO_BIO_T* network_bio;

} co_tls_client_t;

#define co_tcp_client_get_tls(client) \
    ((co_tls_client_t*)client->sock.tls)

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_tls_client_setup(
    co_tls_client_t* tls,
    co_tls_ctx_st* tls_ctx,
    co_tcp_client_t* client
);

void
co_tls_client_cleanup(
    co_tls_client_t* tls
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_TLS_API
co_tcp_client_t*
co_tls_client_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
);

CO_TLS_API
void
co_tls_client_destroy(
    co_tcp_client_t* client
);

CO_TLS_API
co_tls_callbacks_st*
co_tls_get_callbacks(
    co_tcp_client_t* client
);

CO_TLS_API
void
co_tls_set_host_name(
    co_tcp_client_t* client,
    const char* host_name
);

CO_TLS_API
void
co_tls_set_available_protocols(
    co_tcp_client_t* client,
    const char* protocols[],
    size_t count
);

CO_TLS_API
bool
co_tls_get_selected_protocol(
    const co_tcp_client_t* client,
    char* buffer,
    size_t buffer_size
);

CO_TLS_API
bool
co_tls_start_handshake(
    co_tcp_client_t* client
);

CO_TLS_API
bool
co_tls_send(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size
);

CO_TLS_API
bool
co_tls_send_async(
    co_tcp_client_t* client,
    const void* data,
    size_t data_size,
    void* user_data
);

CO_TLS_API
ssize_t
co_tls_receive(
    co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size
);

CO_TLS_API
ssize_t
co_tls_receive_all(
    co_tcp_client_t* client,
    co_byte_array_t* byte_array
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_CLIENT_H_INCLUDED
