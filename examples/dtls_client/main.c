#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#   ifdef CO_USE_WOLFSSL
#       pragma comment(lib, "wolfssl.lib")
#   elif defined(CO_USE_OPENSSL)
#       pragma comment(lib, "libssl.lib")
#       pragma comment(lib, "libcrypto.lib")
#   endif
#endif

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_udp_t* client;
    co_net_addr_t remote_net_addr;

} my_app;

void on_my_receive(my_app* self, co_udp_t* client)
{
    (void)self;

    for (;;)
    {
        char buffer[1024];

        ssize_t size = co_dtls_udp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        printf("receive %zd bytes\n", (size_t)size);
    }

    co_udp_restart_receive_timer(client);
}

void on_my_receive_timer(my_app* self, co_udp_t* client)
{
    printf("receive timeout\n");

    co_dtls_udp_client_destroy(client);
    self->client = NULL;

    // quit app
    co_app_stop();
}

void on_my_handshake(my_app* self, co_udp_t* client, int error_code)
{
    if (error_code == 0)
    {
        printf("handshake success\n");

        // send
        const char* data = "hello";
        co_dtls_udp_send(client, data, strlen(data) + 1);

        co_udp_start_receive_timer(client);
    }
    else
    {
        printf("handshake failed\n");

        co_dtls_udp_client_destroy(client);
        self->client = NULL;

        // quit app
        co_app_stop();
    }
}

#ifdef CO_USE_TLS

#ifndef CO_USE_WOLFSSL
// TODO
int on_my_verify_cookie(SSL* ssl, const unsigned char* cookie, unsigned int cookie_len)
{
    (void)ssl;
    (void)cookie;
    (void)cookie_len;

    // ok
    return 1;
}
#endif

int on_my_verify_peer(int preverify_ok, X509_STORE_CTX* x509_ctx)
{
    (void)preverify_ok;
    (void)x509_ctx;

    // ok
    return 1;
}

#endif

bool my_tls_setup(co_tls_ctx_st* tls_ctx)
{
#ifdef CO_USE_TLS

#ifdef CO_USE_WOLFSSL
    SSL_CTX* ssl_ctx = SSL_CTX_new(wolfDTLS_client_method());
#else
    SSL_CTX* ssl_ctx = SSL_CTX_new(DTLS_client_method());
#endif

    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, on_my_verify_peer);

#ifndef CO_USE_WOLFSSL
    // TODO
    SSL_CTX_set_cookie_verify_cb(ssl_ctx, on_my_verify_cookie);
#endif

    tls_ctx->ssl_ctx = ssl_ctx;

#endif

    return true;
}

bool on_my_app_create(my_app* self)
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
    co_net_addr_set_family(&local_net_addr, co_net_addr_get_family(&self->remote_net_addr));

    co_tls_ctx_st tls_ctx = { 0 };
    my_tls_setup(&tls_ctx);

    self->client = co_dtls_udp_client_create(&local_net_addr, &tls_ctx);

    if (self->client == NULL)
    {
        printf("Failed to create tls client (maybe SSL library was not found)\n");

        return false;
    }

    // callback
    co_udp_callbacks_st* udp_callbacks = co_udp_get_callbacks(self->client);
    udp_callbacks->on_receive = (co_udp_receive_fn)on_my_receive;
    udp_callbacks->on_receive_timer = (co_udp_receive_timer_fn)on_my_receive_timer;
    co_dtls_udp_callbacks_st* tls_callbacks = co_dtls_udp_get_callbacks(self->client);
    tls_callbacks->on_handshake = (co_dtls_udp_handshake_fn)on_my_handshake;

    // handshake
    co_dtls_udp_handshake_start(self->client, &self->remote_net_addr);

    char remote_str[64];
    co_net_addr_to_string(&self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("handshake with %s\n", remote_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_dtls_udp_client_destroy(self->client);
}

int main(int argc, char* argv[])
{
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_udp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    return co_net_app_start(
        (co_app_t*)&app, "my_app",
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);
}
