#ifndef CO_TLS_CLIENT_H_INCLUDED
#define CO_TLS_CLIENT_H_INCLUDED

#include <coldforce/core/co_byte_array.h>
#include <coldforce/core/co_queue.h>
#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_socket.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef void(*co_tls_handshake_fn)(
    co_thread_t* self, co_socket_t* sock, int error_code);

typedef struct
{
    co_tls_handshake_fn on_handshake;

} co_tls_callbacks_st;

typedef struct
{
    co_tls_ctx_st ctx;
    co_tls_callbacks_st callbacks;

    void* on_receive_origin;
    co_timer_t* handshake_timer;

    co_byte_array_t* send_data;
    co_queue_t* receive_data_queue;

    CO_SSL_T* ssl;
    CO_BIO_T* network_bio;

} co_tls_client_t;

#define CO_TLS_COOKIE_MAX_LENGTH       255

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#ifdef CO_USE_OPENSSL_COMPATIBLE

void
co_tls_on_handshake_receive(
    co_thread_t* thread,
    co_socket_t* sock
);

bool
co_tls_handshake_receive(
    co_thread_t* thread,
    co_socket_t* sock
);

bool
co_tls_client_setup_internal(
    co_tls_client_t* tls,
    co_tls_ctx_st* tls_ctx,
    co_socket_t* sock
);

void
co_tls_client_cleanup_internal(
    co_tls_client_t* tls
);

bool
co_tls_client_setup(
    co_socket_t* sock,
    co_tls_ctx_st* tls_ctx
);

void
co_tls_client_cleanup(
    co_socket_t* sock
);

bool
co_tls_handshake_start(
    co_socket_t* sock,
    uint32_t timeout_msec
);

bool
co_tls_encrypt_data(
    co_socket_t* sock,
    const void* plain_data,
    size_t plain_data_size,
    co_byte_array_t* enc_data
);

ssize_t
co_tls_decrypt_data(
    co_socket_t* sock,
    co_queue_t* enc_data,
    uint8_t* plain_buffer,
    size_t plain_buffer_size
);

#endif // CO_USE_OPENSSL_COMPATIBLE

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

#ifdef CO_USE_OPENSSL_COMPATIBLE

CO_TLS_API
co_socket_t*
co_tls_get_socket(
    const CO_SSL_T* ssl
);

CO_TLS_API
co_thread_t*
co_tls_get_thread(
    const CO_SSL_T* ssl
);

CO_TLS_API
size_t
co_tls_genelate_cookie(
    const CO_SSL_T* ssl,
    const uint8_t* cookie_secret,
    size_t cookie_secret_size,
    uint8_t* buffer,
    size_t buffer_size
);

#endif // CO_USE_OPENSSL_COMPATIBLE

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_CLIENT_H_INCLUDED
