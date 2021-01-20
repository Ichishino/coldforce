#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/tls/co_tls_tcp_server.h>
#include <coldforce/tls/co_tls_tcp_client.h>

//---------------------------------------------------------------------------//
// tls tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static void
co_tls_tcp_server_setup(
    co_tls_tcp_server_t* tls,
    co_tls_ctx_st* tls_ctx
)
{
    if ((tls_ctx == NULL) || (tls_ctx->ssl_ctx == NULL))
    {
        tls->ctx.ssl_ctx = SSL_CTX_new(TLS_server_method());
    }
    else
    {
        tls->ctx.ssl_ctx = tls_ctx->ssl_ctx;
    }

    tls->protocols = NULL;
    tls->protocols_length = 0;

    tls->on_accept_ready = NULL;
}

static void
co_tls_tcp_server_cleanup(
    co_tls_tcp_server_t* tls
)
{
    if (tls != NULL)
    {
        co_mem_free(tls->protocols);
        tls->protocols = NULL;
        tls->protocols_length = 0;

        SSL_CTX_free(tls->ctx.ssl_ctx);
        tls->ctx.ssl_ctx = NULL;
    }
}

static void
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

    client->sock.tls = client_tls;

    co_tls_tcp_server_t* server_tls = co_tcp_server_get_tls(server);

    SSL_CTX_up_ref(server_tls->ctx.ssl_ctx);

    co_tls_tcp_client_setup(client_tls, &server_tls->ctx);
    SSL_set_accept_state(client_tls->ssl);

    if (server_tls->on_accept_ready != NULL)
    {
        server_tls->on_accept_ready(thread, server, client);
    }
}

static int
co_tls_tcp_server_on_alpn_select(
    SSL* ssl,
    const unsigned char** out,
    unsigned char* outlen,
    const unsigned char* in,
    unsigned int inlen,
    void* arg
)
{
    (void)ssl;

    co_tls_tcp_server_t* tls = (co_tls_tcp_server_t*)arg;

    const uint8_t* protocols1 = tls->protocols;

    while ((uintptr_t)
        (protocols1 - tls->protocols) < tls->protocols_length)
    {
        uint8_t length1 = *protocols1;
        ++protocols1;

        const uint8_t* protocols2 = in;

        while ((unsigned int)(protocols2 - in) < inlen)
        {
            uint8_t length2 = *protocols2;
            ++protocols2;

            if ((length1 == length2) &&
                (memcmp(protocols1, protocols2, length1) == 0))
            {
                (*out) = protocols2;
                (*outlen) = length2;

                return SSL_TLSEXT_ERR_OK;
            }

            protocols2 += length2;
        }

        protocols1 += length1;
    }

    return SSL_TLSEXT_ERR_NOACK;
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

    server->sock.tls = tls;

    return server;
}

void
co_tls_tcp_server_destroy(
    co_tcp_server_t* server
)
{
    if (server != NULL)
    {
        co_tls_tcp_server_cleanup(server->sock.tls);
        co_mem_free(server->sock.tls);

        co_tcp_server_destroy(server);
    }
}

void
co_tls_tcp_server_close(
    co_tcp_server_t* server
)
{
    if (server != NULL)
    {
        co_tcp_server_close(server);
    }
}

void
co_tls_tcp_server_set_alpn_available_protocols(
    co_tcp_server_t* server,
    const char* protocols[],
    size_t count
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    for (size_t index = 0; index < count; ++index)
    {
        uint8_t length = (uint8_t)strlen(protocols[index]);

        co_byte_array_add(buffer, &length, 1);
        co_byte_array_add(buffer, protocols[index], length);
    }

    co_tls_tcp_server_t* tls = co_tcp_server_get_tls(server);

    SSL_CTX_set_alpn_select_cb(
        tls->ctx.ssl_ctx, co_tls_tcp_server_on_alpn_select, tls);

    tls->protocols_length = co_byte_array_get_count(buffer);
    tls->protocols = co_byte_array_detach(buffer);

    co_byte_array_destroy(buffer);
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
