#ifndef CO_TLS_SERVER_H_INCLUDED
#define CO_TLS_SERVER_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_socket.h>

#include <coldforce/tls/co_tls.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tls server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef void(*co_tls_accept_fn)(
    co_thread_t* self, co_socket_t* sock_server, co_socket_t* sock_client);

typedef struct
{
    co_tls_ctx_st ctx;

    size_t protocols_length;
    uint8_t* protocols;

    co_tls_accept_fn on_accept;

} co_tls_server_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#ifdef CO_USE_OPENSSL_COMPATIBLE

void
co_tls_server_on_accept_ready(
    co_thread_t* thread,
    co_socket_t* sock_server,
    co_socket_t* sock_client
);

bool
co_tls_server_setup(
    co_socket_t* sock_server,
    co_tls_ctx_st* tls_ctx
);

void
co_tls_server_cleanup(
    co_socket_t* sock_server
);

#endif // CO_USE_OPENSSL_COMPATIBLE

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_TLS_SERVER_H_INCLUDED
