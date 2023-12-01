#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_server.h>
#include <coldforce/tls/co_tls_client.h>

//---------------------------------------------------------------------------//
// tls server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#ifdef CO_USE_OPENSSL_COMPATIBLE

void
co_tls_server_on_accept_ready(
    co_thread_t* thread,
    co_socket_t* sock_server,
    co_socket_t* sock_client
)
{
    co_tls_server_t* tls_server =
        (co_tls_server_t*)sock_server->tls;

    SSL_CTX_up_ref(tls_server->ctx.ssl_ctx);

    co_tls_client_t* tls_client =
        (co_tls_client_t*)co_mem_alloc(sizeof(co_tls_client_t));

    co_tls_client_setup_internal(
        tls_client, &tls_server->ctx, sock_client);

    SSL_set_accept_state(tls_client->ssl);

    sock_client->tls = tls_client;

    if (tls_server->on_accept != NULL)
    {
        tls_server->on_accept(thread, sock_server, sock_client);
    }
}

bool
co_tls_server_setup(
    co_socket_t* sock_server,
    co_tls_ctx_st* tls_ctx
)
{
    if ((tls_ctx == NULL) || (tls_ctx->ssl_ctx == NULL))
    {
        return false;
    }

    co_tls_server_t* tls_server =
        (co_tls_server_t*)co_mem_alloc(sizeof(co_tls_server_t));

    if (tls_server == NULL)
    {
        return false;
    }

    tls_server->ctx.ssl_ctx = tls_ctx->ssl_ctx;

    tls_server->protocols = NULL;
    tls_server->protocols_length = 0;

    tls_server->on_accept = NULL;

    sock_server->tls = tls_server;

    return true;
}

void
co_tls_server_cleanup(
    co_socket_t* sock_server
)
{
    if (sock_server != NULL)
    {
        co_tls_server_t* tls_server =
            (co_tls_server_t*)sock_server->tls;

        if (tls_server != NULL)
        {
            SSL_CTX_free(tls_server->ctx.ssl_ctx);
            co_mem_free(tls_server->protocols);
            co_mem_free(tls_server);

            sock_server->tls = NULL;
        }
    }
}

#endif // CO_USE_OPENSSL_COMPATIBLE
