#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_tcp_server.h>
#include <coldforce/tls/co_tls_server.h>

//---------------------------------------------------------------------------//
// tls tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#ifdef CO_USE_OPENSSL_COMPATIBLE

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

    co_tls_server_t* tls_server = (co_tls_server_t*)arg;

    const uint8_t* protocols1 = tls_server->protocols;

    while ((uintptr_t)
        (protocols1 - tls_server->protocols) < tls_server->protocols_length)
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

#endif // CO_USE_OPENSSL_COMPATIBLE

bool
co_tls_tcp_server_setup(
    co_tcp_server_t* tcp_server,
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS

    if (co_tls_server_setup(&tcp_server->sock, tls_ctx))
    {
        co_tls_server_t* tls_server =
            (co_tls_server_t*)tcp_server->sock.tls;

        tls_server->on_accept =
            (co_tls_accept_fn)tcp_server->callbacks.on_accept;
        tcp_server->callbacks.on_accept =
            (co_tcp_accept_fn)co_tls_server_on_accept_ready;

        return true;
    }

    return false;

#else

    (void)tcp_server;
    (void)tls_ctx;

    return false;

#endif // CO_USE_TLS
}

void
co_tls_tcp_server_cleanup(
    co_tcp_server_t* tcp_server
)
{
#ifdef CO_USE_TLS

    if (tcp_server != NULL)
    {
        co_tls_server_cleanup(&tcp_server->sock);
    }

#else

    (void)tcp_server;

#endif // CO_USE_TLS
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_tcp_server_t*
co_tls_tcp_server_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_tcp_server_t* tcp_server =
        co_tcp_server_create(local_net_addr);

    if (tcp_server == NULL)
    {
        return NULL;
    }

    if (!co_tls_tcp_server_setup(tcp_server, tls_ctx))
    {
        co_tcp_server_destroy(tcp_server);

        return NULL;
    }

    return tcp_server;
}

void
co_tls_tcp_server_destroy(
    co_tcp_server_t* tcp_server
)
{
    if (tcp_server != NULL)
    {
        co_tls_tcp_server_cleanup(tcp_server);
        co_tcp_server_destroy(tcp_server);
    }
}

bool
co_tls_tcp_server_start(
    co_tcp_server_t* tcp_server,
    int backlog
)
{
#ifdef CO_USE_TLS

    co_tls_server_t* tls =
        (co_tls_server_t*)tcp_server->sock.tls;

    tls->on_accept =
        (co_tls_accept_fn)tcp_server->callbacks.on_accept;
    tcp_server->callbacks.on_accept =
        (co_tcp_accept_fn)co_tls_server_on_accept_ready;

    return co_tcp_server_start(tcp_server, backlog);

#else

    (void)tcp_server;
    (void)backlog;

    return false;

#endif // CO_USE_TLS
}

void
co_tls_tcp_server_set_available_protocols(
    co_tcp_server_t* tcp_server,
    const char* protocols[],
    size_t count
)
{
#ifdef CO_USE_OPENSSL_COMPATIBLE

    co_byte_array_t* buffer = co_byte_array_create();

    for (size_t index = 0; index < count; ++index)
    {
        uint8_t length = (uint8_t)strlen(protocols[index]);

        co_byte_array_add(buffer, &length, 1);
        co_byte_array_add(buffer, protocols[index], length);
    }

    co_tls_server_t* tls_server =
        (co_tls_server_t*)tcp_server->sock.tls;

    SSL_CTX_set_alpn_select_cb(
        tls_server->ctx.ssl_ctx,
        co_tls_tcp_server_on_alpn_select,
        tls_server);

    tls_server->protocols_length = co_byte_array_get_count(buffer);
    tls_server->protocols = co_byte_array_detach(buffer);

    co_byte_array_destroy(buffer);

#else

    (void)tcp_server;
    (void)protocols;
    (void)count;

#endif // CO_USE_OPENSSL_COMPATIBLE
}
