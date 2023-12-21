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
    co_udp_server_t* udp_server;
    co_list_t* udp_clients;
    uint8_t cookie_secret[32];

} app_st;

#define app_get_remote_address(protocol, net_unit, buffer) \
    co_net_addr_to_string( \
        co_socket_get_remote_net_addr( \
            co_##protocol##_get_socket(net_unit)), \
        buffer, sizeof(buffer));

void
app_on_tls_handshake(
    app_st* self,
    co_udp_t* udp_client,
    int error_code
);

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

    char remote_str[64];
    app_get_remote_address(udp, udp_client, remote_str);

    for (;;)
    {
        uint8_t buffer[1024];

        // receive data
        ssize_t data_size =
            co_dtls_udp_receive(udp_client, buffer, sizeof(buffer));

        if (data_size <= 0)
        {
            break;
        }

        printf("receive %zd bytes from %s\n",
            (size_t)data_size, remote_str);

        // send (echo)
        co_dtls_udp_send(udp_client, buffer, data_size);
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
    // receive timeout

    char remote_str[64];
    app_get_remote_address(udp, udp_client, remote_str);
    printf("receive timeout: %s\n", remote_str);

    // close
    co_list_remove(self->udp_clients, udp_client);
}

void
app_on_udp_accept(
    app_st* self,
    co_udp_server_t* udp_server,
    co_udp_t* udp_client
)
{
    (void)udp_server;

    char remote_str[64];
    app_get_remote_address(udp, udp_client, remote_str);
    printf("udp accept: %s\n", remote_str);

    // accept
    co_udp_accept((co_thread_t*)self, udp_client);

    // callbacks
    co_udp_callbacks_st* udp_callbacks = co_udp_get_callbacks(udp_client);
    udp_callbacks->on_receive = (co_udp_receive_fn)app_on_udp_receive;
    udp_callbacks->on_timer = (co_udp_timer_fn)app_on_udp_receive_timer;
    co_tls_callbacks_st* tls_callbacks = co_dtls_udp_get_callbacks(udp_client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)app_on_tls_handshake;

    // start handshake
    if (!co_dtls_udp_handshake_start(udp_client, NULL))
    {
        printf("handshake failed: %s\n", remote_str);

        co_dtls_udp_client_destroy(udp_client);

        return;
    }

    co_list_add_tail(self->udp_clients, udp_client);
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
    char remote_str[64];
    app_get_remote_address(udp, udp_client, remote_str);

    if (error_code == 0)
    {
        printf("handshake success: %s\n", remote_str);

        // start receive timer
        co_udp_create_timer(udp_client, 60*1000); // 1min
        co_udp_start_timer(udp_client);
    }
    else
    {
        printf("handshake failed: %s\n", remote_str);

        // close
        co_list_remove(self->udp_clients, udp_client);
    }
}

#if defined(CO_USE_TLS) && !defined(CO_USE_WOLFSSL)
int
app_on_tls_generate_cookie(
    SSL* ssl,
    unsigned char* cookie,
    unsigned int* cookie_length
)
{
    app_st* self = (app_st*)co_tls_get_thread(ssl);

    size_t length =
        co_tls_genelate_cookie(ssl,
            self->cookie_secret, sizeof(self->cookie_secret),
            cookie, CO_TLS_COOKIE_MAX_LENGTH);

    *cookie_length = (unsigned int)length;

    return (int)(length > 0);
}

int
app_on_tls_verify_cookie(
    SSL* ssl,
    const unsigned char* cookie,
    unsigned int cookie_length
)
{
    app_st* self = (app_st*)co_tls_get_thread(ssl);

    uint8_t verify_cookie[CO_TLS_COOKIE_MAX_LENGTH];

    size_t verify_cookie_length =
        co_tls_genelate_cookie(ssl,
            self->cookie_secret, sizeof(self->cookie_secret),
            verify_cookie, sizeof(verify_cookie));

    if ((verify_cookie_length != cookie_length) ||
        (memcmp(cookie, verify_cookie, cookie_length) != 0))
    {
        printf("verify cookie: error\n");

        return 0;
    }

    printf("verify cookie: ok\n");

    return 1;
}
#endif

//---------------------------------------------------------------------------//
// tls setup
//---------------------------------------------------------------------------//

bool
app_tls_setup(
    app_st* self,
    co_tls_ctx_st* tls_ctx
)
{
#ifdef CO_USE_TLS
    const char* certificate_file = "../../../test_file/server.crt";
    const char* private_key_file = "../../../test_file/server.key";

    #ifdef CO_USE_WOLFSSL
    SSL_CTX* ssl_ctx = SSL_CTX_new(wolfDTLS_server_method());
    #else
    SSL_CTX* ssl_ctx = SSL_CTX_new(DTLS_server_method());
    #endif

    if (SSL_CTX_use_certificate_file(
        ssl_ctx, certificate_file, SSL_FILETYPE_PEM) != 1)
    {
        SSL_CTX_free(ssl_ctx);

        printf("SSL_CTX_use_certificate_file failed\n");

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(
        ssl_ctx, private_key_file, SSL_FILETYPE_PEM) != 1)
    {
        SSL_CTX_free(ssl_ctx);

        printf("SSL_CTX_use_PrivateKey_file failed\n");

        return false;
    }

    co_random(
        self->cookie_secret,
        sizeof(self->cookie_secret));

    SSL_CTX_set_options(
        ssl_ctx, SSL_OP_COOKIE_EXCHANGE);

    #ifndef CO_USE_WOLFSSL
    SSL_CTX_set_cookie_generate_cb(
        ssl_ctx, app_on_tls_generate_cookie);
    SSL_CTX_set_cookie_verify_cb(
        ssl_ctx, app_on_tls_verify_cookie);
    #endif

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

    if (args->count <= 1)
    {
        printf("<Usage>\n");
        printf("dtls_server <port_number>\n");

        return false;
    }

    // port
    uint16_t port = (uint16_t)atoi(args->values[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value =
        (co_item_destroy_fn)co_dtls_udp_client_destroy; // auto destroy
    self->udp_clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);
    
    // tls setup
    co_tls_ctx_st tls_ctx = { 0 };
    if (!app_tls_setup(self, &tls_ctx))
    {
        return false;
    }

    // create dtls udp server
    self->udp_server =
        co_dtls_udp_server_create(&local_net_addr, &tls_ctx);
    if (self->udp_server == NULL)
    {
        printf("Failed to create tls server (maybe SSL/TLS library was not found)\n");

        return false;
    }

    // socket option
    co_socket_option_set_reuse_addr(
        co_udp_server_get_socket(self->udp_server), true);

    // callbacks
    co_udp_server_callbacks_st* callbacks =
        co_udp_server_get_callbacks(self->udp_server);
    callbacks->on_accept = (co_udp_accept_fn)app_on_udp_accept;

    // start dtls udp server
    if (!co_dtls_udp_server_start(self->udp_server))
    {
        printf("Failed to start server\n");

        return false;
    }

    char local_str[64];
    co_net_addr_to_string(
        &local_net_addr, local_str, sizeof(local_str));
    printf("start server: %s\n", local_str);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_list_destroy(self->udp_clients);
    co_dtls_udp_server_destroy(self->udp_server);
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
        (co_app_t*)&self, "dtls-server-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
