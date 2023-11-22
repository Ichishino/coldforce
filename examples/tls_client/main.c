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
    co_tcp_client_t* client;
    co_net_addr_t remote_net_addr;
    co_timer_t* retry_timer;

} my_app;

bool my_connect(my_app* self);

void on_my_receive(my_app* self, co_tcp_client_t* client)
{
    (void)self;

    for (;;)
    {
        char buffer[1024];

        ssize_t size = co_tls_tcp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        printf("receive %zd bytes\n", (size_t)size);
    }
}

#ifdef CO_USE_TLS
int on_my_verify_peer(int preverify_ok, X509_STORE_CTX* x509_ctx)
{
    (void)preverify_ok;
    (void)x509_ctx;

    // always OK for debug
    return 1;
}
#endif

void on_my_handshake(my_app* self, co_tcp_client_t* client, int error_code)
{
    if (error_code == 0)
    {
        printf("handshake success\n");

        // send
        const char* data = "hello";
        co_tls_tcp_send(client, data, strlen(data) + 1);
    }
    else
    {
        printf("handshake failed\n");

        co_tls_tcp_client_destroy(self->client);
        self->client = NULL;

        // quit app
        co_app_stop();
    }
}

void on_my_close(my_app* self, co_tcp_client_t* client)
{
    (void)client;

    printf("close\n");

    co_tls_tcp_client_destroy(self->client);
    self->client = NULL;

    // quit app
    co_app_stop();
}

void on_my_connect(my_app* self, co_tcp_client_t* client, int error_code)
{
    if (error_code == 0)
    {
        printf("connect success\n");
        printf("handshake start\n");

        // handshake
        co_tls_tcp_start_handshake(client);
    }
    else
    {
        printf("connect failed\n");

        co_tls_tcp_client_destroy(self->client);
        self->client = NULL;

        // start retry timer
        co_timer_start(self->retry_timer);
    }
}

bool my_connect(my_app* self)
{
    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, co_net_addr_get_family(&self->remote_net_addr));

    co_tls_ctx_st tls_ctx = { 0 };

#ifdef CO_USE_TLS
    SSL_CTX* ssl_ctx = SSL_CTX_new(TLS_client_method());
    SSL_CTX_set_default_verify_paths(ssl_ctx);
#if defined(CO_USE_WOLFSSL) && defined(_WIN32)
    SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF); // TODO
#endif
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, on_my_verify_peer);
    tls_ctx.ssl_ctx = ssl_ctx;
#endif

    self->client = co_tls_tcp_client_create(&local_net_addr, &tls_ctx);

    if (self->client == NULL)
    {
        printf("Failed to create tls client (maybe SSL library was not found)\n");

        return false;
    }

    // callback
    co_tcp_callbacks_st* tcp_callbacks = co_tcp_get_callbacks(self->client);
    tcp_callbacks->on_connect = (co_tcp_connect_fn)on_my_connect;
    tcp_callbacks->on_receive = (co_tcp_receive_fn)on_my_receive;
    tcp_callbacks->on_close = (co_tcp_close_fn)on_my_close;
    co_tls_tcp_callbacks_st* tls_callbacks = co_tls_tcp_get_callbacks(self->client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)on_my_handshake;

    // connect
    co_tcp_connect(self->client, &self->remote_net_addr);

    char remote_str[64];
    co_net_addr_to_string(&self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("connect to %s\n", remote_str);

    return true;
}

void on_my_retry_timer(my_app* self, co_timer_t* timer)
{
    (void)timer;

    // connect retry
    my_connect(self);
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
        printf("tls_client <ip_address:port>\n");

        return false;
    }

    // connect retry timer
    self->retry_timer = co_timer_create(
        5000, (co_timer_fn)on_my_retry_timer, false, 0);

    // connect
    if (!my_connect(self))
    {
        return false;
    }

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_timer_destroy(self->retry_timer);
    co_tls_tcp_client_destroy(self->client);
}

int main(int argc, char* argv[])
{
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    return co_net_app_start(
        (co_app_t*)&app, "my_app",
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);
}
