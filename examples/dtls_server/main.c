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
    co_udp_server_t* server;
    co_list_t* client_list;

} my_app;

void on_my_dtls_handshake(my_app* self, co_udp_t* client, int error_code)
{
    if (error_code == 0)
    {
        printf("handshake success\n");

        // can send and receive
    }
    else
    {
        printf("handshake failed\n");

        co_list_remove(self->client_list, client);
    }
}

void on_my_udp_receive(my_app* self, co_udp_t* client)
{
    (void)self;

    for (;;)
    {
        uint8_t buffer[1024];

        // receive
        ssize_t data_size = co_dtls_udp_receive(client, buffer, sizeof(buffer));

        if (data_size <= 0)
        {
            break;
        }

        printf("receive %zd bytes\n", (size_t)data_size);

        // send (echo)
        co_dtls_udp_send(client, buffer, data_size);
    }
}

int on_my_generate_cookie(SSL* ssl, unsigned char* cookie, unsigned int* cookie_len)
{
    (void)ssl;

    memcpy(cookie, "mycookie", 8);
    *cookie_len = 8;

    return 1;
}

bool my_tls_setup(co_tls_ctx_st* tls_ctx)
{
#ifdef CO_USE_TLS
    const char* certificate_file = "../../../test_file/server.crt";
    const char* private_key_file = "../../../test_file/server.key";

    SSL_CTX* ssl_ctx = SSL_CTX_new(DTLS_server_method());

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

    SSL_CTX_set_cookie_generate_cb(ssl_ctx, on_my_generate_cookie);

    tls_ctx->ssl_ctx = ssl_ctx;
#endif

    return true;
}

void on_my_udp_accept(my_app* self, co_udp_server_t* server, co_udp_t* client)
{
    (void)server;

    // accept
    co_udp_accept((co_thread_t*)self, client);

    // udp callback
    co_udp_callbacks_st* udp_callbacks = co_udp_get_callbacks(client);
    udp_callbacks->on_receive = (co_udp_receive_fn)on_my_udp_receive;

    // dtls callback
    co_dtls_udp_callbacks_st* dtls_callbacks = co_dtls_udp_get_callbacks(client);
    dtls_callbacks->on_handshake = (co_dtls_udp_handshake_fn)on_my_dtls_handshake;

    // dtls handshake
    if (!co_dtls_udp_start_handshake(client, NULL))
    {
        printf("handshake failed\n");

        co_list_remove(self->client_list, client);

        return;
    }

    co_list_add_tail(self->client_list, client);

    char remote_str[64];
    co_net_addr_to_string(
        co_socket_get_remote_net_addr(co_udp_get_socket(client)), remote_str, sizeof(remote_str));
    printf("accept %s\n", remote_str);
}

bool on_my_app_create(my_app* self)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count <= 1)
    {
        printf("<Usage>\n");
        printf("dtls_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_dtls_udp_client_destroy; // auto destroy
    self->client_list = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);
    
    // dtls setting
    co_tls_ctx_st tls_ctx = { 0 };
    if (!my_tls_setup(&tls_ctx))
    {
        return false;
    }

    // dtls server
    self->server = co_dtls_udp_server_create(&local_net_addr, &tls_ctx);
    if (self->server == NULL)
    {
        printf("Failed to create tls server (maybe SSL library was not found)\n");

        return false;
    }

    // socket option
    co_socket_option_set_reuse_addr(
        co_udp_server_get_socket(self->server), true);

    // callback
    co_udp_server_callbacks_st* callbacks = co_udp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_udp_accept_fn)on_my_udp_accept;

    // server start
    co_dtls_udp_server_start(self->server);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("server %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->client_list);
    co_dtls_udp_server_destroy(self->server);
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
