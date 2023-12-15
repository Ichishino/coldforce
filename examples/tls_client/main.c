#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef _WIN32
#   ifdef CO_USE_WOLFSSL
#       pragma comment(lib, "wolfssl.lib")
#   elif defined(CO_USE_OPENSSL)
#       pragma comment(lib, "libssl.lib")
#       pragma comment(lib, "libcrypto.lib")
#   endif
#endif

//---------------------------------------------------------------------------//
// app object
//---------------------------------------------------------------------------//

typedef struct
{
    co_app_t base_app;

    // app data
    co_tcp_client_t* tcp_client;
    co_net_addr_t remote_net_addr;
    co_timer_t* retry_timer;

} app_st;

//---------------------------------------------------------------------------//
// tcp callback
//---------------------------------------------------------------------------//

void
app_on_tcp_receive(
    app_st* self,
    co_tcp_client_t* tcp_client
)
{
    (void)self;

    for (;;)
    {
        char buffer[1024];

        // receive data
        ssize_t size =
            co_tls_tcp_receive(tcp_client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        printf("receive %zd bytes\n", (size_t)size);
    }
}

void
app_on_tcp_close(
    app_st* self,
    co_tcp_client_t* tcp_client
)
{
    printf("closed\n");

    co_tls_tcp_client_destroy(tcp_client);
    self->tcp_client = NULL;

    // quit app
    co_app_stop();
}

void
app_on_tcp_connect(
    app_st* self,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    if (error_code == 0)
    {
        printf("connect success\n");
        printf("start tls handshake\n");

        // start tls handshake
        co_tls_tcp_handshake_start(tcp_client);
    }
    else
    {
        printf("connect failed: %d\n", error_code);

        co_tls_tcp_client_destroy(tcp_client);
        self->tcp_client = NULL;

        // start retry timer
        co_timer_start(self->retry_timer);
    }
}

//---------------------------------------------------------------------------//
// tls callback
//---------------------------------------------------------------------------//

#ifdef CO_USE_TLS
int
app_on_tls_verify_peer(
    int preverify_ok,
    X509_STORE_CTX* x509_ctx
)
{
    (void)preverify_ok;
    (void)x509_ctx;

    // always OK for debug
    return 1;
}
#endif

void
app_on_tls_handshake(
    app_st* self,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    if (error_code == 0)
    {
        printf("tls handshake success\n");

        // send data
        const char* data = "hello";
        co_tls_tcp_send(tcp_client, data, strlen(data) + 1);
    }
    else
    {
        printf("tls handshake failed\n");

        co_tls_tcp_client_destroy(tcp_client);
        self->tcp_client = NULL;

        // quit app
        co_app_stop();
    }
}

//---------------------------------------------------------------------------//
// tls tcp connect
//---------------------------------------------------------------------------//

bool
app_tls_tcp_connect(
    app_st* self
)
{
    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(
        &local_net_addr, co_net_addr_get_family(&self->remote_net_addr));

    co_tls_ctx_st tls_ctx = { 0 };

    // tls setup
#ifdef CO_USE_TLS
    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_default_verify_paths(ssl_ctx);
#if defined(CO_USE_WOLFSSL) && defined(_WIN32)
    SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF); // TODO
#endif
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, app_on_tls_verify_peer);
    tls_ctx.ssl_ctx = ssl_ctx;
#endif

    // create tls tcp client
    self->tcp_client =
        co_tls_tcp_client_create(&local_net_addr, &tls_ctx);

    if (self->tcp_client == NULL)
    {
        printf("Failed to create tls client (maybe SSL/TLS library was not found)\n");

        return false;
    }

    // callbacks
    co_tcp_callbacks_st* tcp_callbacks = co_tcp_get_callbacks(self->tcp_client);
    tcp_callbacks->on_connect = (co_tcp_connect_fn)app_on_tcp_connect;
    tcp_callbacks->on_receive = (co_tcp_receive_fn)app_on_tcp_receive;
    tcp_callbacks->on_close = (co_tcp_close_fn)app_on_tcp_close;
    co_tls_callbacks_st* tls_callbacks = co_tls_tcp_get_callbacks(self->tcp_client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)app_on_tls_handshake;

    // start connect
    co_tcp_connect_start(self->tcp_client, &self->remote_net_addr);

    char remote_str[64];
    co_net_addr_to_string(&self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("connect to %s\n", remote_str);

    return true;
}

//---------------------------------------------------------------------------//
// timer callback
//---------------------------------------------------------------------------//

void
app_on_retry_timer(
    app_st* self,
    co_timer_t* timer
)
{
    (void)timer;

    // connect retry
    app_tls_tcp_connect(self);
}

//---------------------------------------------------------------------------//
// app callback
//---------------------------------------------------------------------------//

bool
app_on_create(
    app_st* self
)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count < 2 ||
        !co_net_addr_from_string(
            CO_NET_ADDR_FAMILY_IPV4, args->values[1],
            &self->remote_net_addr))
    {
        printf("<Usage>\n");
        printf("tls_client <ip_address:port>\n");

        return false;
    }

    // create connect retry timer
    self->retry_timer = co_timer_create(
        5000, (co_timer_fn)app_on_retry_timer, false, 0);

    // start connect
    if (!app_tls_tcp_connect(self))
    {
        return false;
    }

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_tls_tcp_client_destroy(self->tcp_client);
    co_timer_destroy(self->retry_timer);
}

void
app_on_signal(
    int sig
)
{
    (void)sig;

    // quit app
    co_app_stop();
}

//---------------------------------------------------------------------------//
// main
//---------------------------------------------------------------------------//

int
main(
    int argc,
    char* argv[]
)
{
    co_win_debug_crt_set_flags();

    signal(SIGINT, app_on_signal);

//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "tls-client-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
