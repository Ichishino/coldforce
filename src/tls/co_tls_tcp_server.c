#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_tcp_server.h>
#include <coldforce/tls/co_tls_tcp_client.h>

//---------------------------------------------------------------------------//
// tls tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_tls_tcp_server_setup(
    co_tls_tcp_server_t* tls,
    co_tls_ctx_st* tls_ctx
)
{
    if ((tls_ctx == NULL) || (tls_ctx->ssl_ctx == NULL))
    {
        tls->ctx.ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    }
    else
    {
        tls->ctx.ssl_ctx = tls_ctx->ssl_ctx;
    }
}

void
co_tls_tcp_server_cleanup(
    co_tls_tcp_server_t* tls
)
{
    if (tls != NULL)
    {
        SSL_CTX_free(tls->ctx.ssl_ctx);
        tls->ctx.ssl_ctx = NULL;
    }
}

void
co_tls_tcp_server_on_accept_ready(
    co_thread_t* thread,
    co_tcp_server_t* server,
    co_tcp_client_t* client
)
{
    co_tls_tcp_client_t* client_tls =
        (co_tls_tcp_client_t*)co_mem_alloc(sizeof(co_tls_tcp_client_t));

    if (client_tls == NULL)
    {
        return;
    }

    client->tls = client_tls;

    co_tls_tcp_server_t* server_tls = co_tcp_server_get_tls(server);

    SSL_CTX_up_ref(server_tls->ctx.ssl_ctx);

    co_tls_tcp_client_setup(client_tls, &server_tls->ctx);
    SSL_set_accept_state(client_tls->ssl);

    if (server_tls->on_accept_ready != NULL)
    {
        server_tls->on_accept_ready(thread, server, client);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_tcp_server_t*
co_tls_tcp_server_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_tcp_server_t* server = co_tcp_server_create(local_net_addr);

    if (server == NULL)
    {
        return NULL;
    }

    co_tls_tcp_server_t* tls =
        (co_tls_tcp_server_t*)co_mem_alloc(sizeof(co_tls_tcp_server_t));

    if (tls == NULL)
    {
        co_tcp_server_destroy(server);

        return NULL;
    }

    co_tls_tcp_server_setup(tls, tls_ctx);

    server->tls = tls;

    return server;
}

void
co_tls_tcp_server_destroy(
    co_tcp_server_t* server
)
{
    if (server != NULL)
    {
        co_tls_tcp_server_cleanup(server->tls);
        co_mem_free(server->tls);

        co_tcp_server_destroy(server);
    }
}

bool
co_tls_tcp_server_start(
    co_tcp_server_t* server,
    co_tcp_accept_fn handler,
    int backlog
)
{
    co_tls_tcp_server_t* tls = co_tcp_server_get_tls(server);

    tls->on_accept_ready = handler;

    return co_tcp_server_start(
        server, (co_tcp_accept_fn)co_tls_tcp_server_on_accept_ready, backlog);
}

bool
co_tls_tcp_accept(
    co_thread_t* owner_thread,
    co_tcp_client_t* client
)
{
    return co_tcp_accept(owner_thread, client);
}
