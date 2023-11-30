#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_dtls_udp_server.h>
#include <coldforce/tls/co_tls_server.h>
#include <coldforce/tls/co_tls_log.h>

//---------------------------------------------------------------------------//
// dtls udp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static bool
co_dtls_udp_server_setup(
    co_udp_server_t* udp_server,
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS

    if (co_tls_server_setup(&udp_server->udp.sock, tls_ctx))
    {
        co_tls_server_t* tls_server =
            (co_tls_server_t*)udp_server->udp.sock.tls;

        tls_server->on_accept =
            (co_tls_accept_fn)udp_server->callbacks.on_accept;
        udp_server->callbacks.on_accept =
            (co_udp_accept_fn)co_tls_server_on_accept_ready;

        return true;
    }

    return false;

#else

    (void)udp_server;
    (void)tls_ctx;

    return false;

#endif // CO_USE_TLS
}

static void
co_dtls_udp_server_cleanup(
    co_udp_server_t* udp_server
)
{
#ifdef CO_USE_TLS

    if (udp_server != NULL)
    {
        co_tls_server_cleanup(&udp_server->udp.sock);
    }

#else

    (void)udp_server;

#endif // CO_USE_TLS
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_udp_server_t*
co_dtls_udp_server_create(
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_udp_server_t* udp_server =
        co_udp_server_create(local_net_addr);

    if (udp_server == NULL)
    {
        return NULL;
    }

    if (!co_dtls_udp_server_setup(udp_server, tls_ctx))
    {
        co_udp_server_destroy(udp_server);

        return NULL;
    }

    return udp_server;
}

void
co_dtls_udp_server_destroy(
    co_udp_server_t* udp_server
)
{
    if (udp_server != NULL)
    {
        co_dtls_udp_server_cleanup(udp_server);
        co_udp_server_destroy(udp_server);
    }
}

bool
co_dtls_udp_server_start(
    co_udp_server_t* udp_server
)
{
#ifdef CO_USE_TLS

    co_tls_server_t* tls =
        (co_tls_server_t*)udp_server->udp.sock.tls;

    tls->on_accept =
        (co_tls_accept_fn)udp_server->callbacks.on_accept;
    udp_server->callbacks.on_accept =
        (co_udp_accept_fn)co_tls_server_on_accept_ready;

    return co_udp_server_start(udp_server);

#else

    (void)udp_server;

    return false;

#endif // CO_USE_TLS
}
