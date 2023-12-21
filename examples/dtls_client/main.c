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
    co_udp_t* udp_client;
    co_net_addr_t remote_net_addr;

} app_st;

//---------------------------------------------------------------------------//
// udp callback
//---------------------------------------------------------------------------//

void
app_on_udp_receive(
    app_st* self,
    co_udp_t* udp_client
)
{
    (void)self;

    for (;;)
    {
        char buffer[1024];

        // receive data
        ssize_t size =
            co_dtls_udp_receive(
                udp_client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        printf("receive %zd bytes\n", (size_t)size);
    }

    // restart receive timer
    co_udp_restart_timer(udp_client);
}

void
app_on_udp_receive_timer(
    app_st* self,
    co_udp_t* udp_client
)
{
    printf("receive timeout\n");

    co_dtls_udp_client_destroy(udp_client);
    self->udp_client = NULL;

    // quit app
    co_app_stop();
}

//---------------------------------------------------------------------------//
// tls callback
//---------------------------------------------------------------------------//

void
app_on_tls_handshake(
    app_st* self,
    co_udp_t* udp_client,
    int error_code
)
{
    if (error_code == 0)
    {
        printf("handshake success\n");

        // send data
        const char* data = "hello";
        co_dtls_udp_send(udp_client, data, strlen(data) + 1);

        // start receive timer
        co_udp_start_timer(udp_client);
    }
    else
    {
        printf("handshake failed\n");

        co_dtls_udp_client_destroy(udp_client);
        self->udp_client = NULL;

        // quit app
        co_app_stop();
    }
}

#ifdef CO_USE_TLS
int
app_on_tls_verify_peer(
    int preverify_ok,
    X509_STORE_CTX* x509_ctx
)
{
    (void)preverify_ok;
    (void)x509_ctx;

    // ok
    return 1;
}
#endif


//---------------------------------------------------------------------------//
// tls setup
//---------------------------------------------------------------------------//

bool
app_tls_setup(
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS

    #ifdef CO_USE_WOLFSSL
    SSL_CTX* ssl_ctx = SSL_CTX_new(wolfDTLS_client_method());
    #else
    SSL_CTX* ssl_ctx = SSL_CTX_new(DTLS_client_method());
    #endif

    SSL_CTX_set_verify(
        ssl_ctx, SSL_VERIFY_PEER, app_on_tls_verify_peer);

    tls_ctx->ssl_ctx = ssl_ctx;

#endif

    return true;
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
        printf("dtls_client <ip_address:port>\n");

        return false;
    }

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(
        &local_net_addr, co_net_addr_get_family(&self->remote_net_addr));

    // setup tls
    co_tls_ctx_st tls_ctx = { 0 };
    app_tls_setup(&tls_ctx);

    // create dtls udp client
    self->udp_client =
        co_dtls_udp_client_create(&local_net_addr, &tls_ctx);

    if (self->udp_client == NULL)
    {
        printf("Failed to create dtls udp client (maybe SSL/TLS library was not found)\n");

        return false;
    }

    // create receive timer
    co_udp_create_timer(self->udp_client, 60*1000);

    // callbacks
    co_udp_callbacks_st* udp_callbacks = co_udp_get_callbacks(self->udp_client);
    udp_callbacks->on_receive = (co_udp_receive_fn)app_on_udp_receive;
    udp_callbacks->on_timer = (co_udp_timer_fn)app_on_udp_receive_timer;
    co_tls_callbacks_st* tls_callbacks = co_dtls_udp_get_callbacks(self->udp_client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)app_on_tls_handshake;

    // start tls handshake
    co_dtls_udp_handshake_start(self->udp_client, &self->remote_net_addr);

    char remote_str[64];
    co_net_addr_to_string(
        &self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("start handshake with %s\n", remote_str);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_dtls_udp_client_destroy(self->udp_client);
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
//    co_udp_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "dtls-client-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
