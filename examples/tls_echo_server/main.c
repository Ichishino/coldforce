#include <coldforce/coldforce_tls.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CO_CAN_USE_TLS

// openssl
#ifdef _WIN32
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_tcp_server_t* server;
    co_list_t* client_list;

} my_app;

void on_my_tls_handshake(my_app* self, co_tcp_client_t* client, int error_code)
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

void on_my_tls_receive(my_app* self, co_tcp_client_t* client)
{
    (void)self;

    co_byte_array_t* byte_array = co_byte_array_create();

    // receive
    ssize_t data_size = co_tls_receive_all(client, byte_array);

    if (data_size > 0)
    {
        printf("receive %zd bytes\n", (size_t)data_size);

        unsigned char* data = co_byte_array_get_ptr(byte_array, 0);

        // send (echo)
        co_tls_send(client, data, data_size);
    }

    co_byte_array_destroy(byte_array);
}

void on_my_tls_close(my_app* self, co_tcp_client_t* client)
{
    char remote_str[64];
    co_net_addr_to_string(
        co_tcp_get_remote_net_addr(client), remote_str, sizeof(remote_str));
    printf("close %s\n", remote_str);

    co_list_remove(self->client_list, client);
}

void on_my_tcp_accept(my_app* self, co_tcp_server_t* server, co_tcp_client_t* client)
{
    (void)server;

    // accept
    co_tcp_accept((co_thread_t*)self, client);

    // tcp callback
    co_tcp_callbacks_st* tcp_callbacks = co_tcp_get_callbacks(client);
    tcp_callbacks->on_receive = (co_tcp_receive_fn)on_my_tls_receive;
    tcp_callbacks->on_close = (co_tcp_close_fn)on_my_tls_close;

    // tls callback
    co_tls_callbacks_st* tls_callbacks = co_tls_get_callbacks(client);
    tls_callbacks->on_handshake = (co_tls_handshake_fn)on_my_tls_handshake;

    // TLS handshake
    co_tls_start_handshake(client);

    co_list_add_tail(self->client_list, client);

    char remote_str[64];
    co_net_addr_to_string(
        co_tcp_get_remote_net_addr(client), remote_str, sizeof(remote_str));
    printf("accept %s\n", remote_str);
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    if (arg->argc <= 1)
    {
        printf("<Usage>\n");
        printf("tls_echo_server port_number\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(arg->argv[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tls_client_destroy; // auto destroy
    self->client_list = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);
    
    // TLS setting (openssl)
    co_tls_ctx_st tls_ctx;
    tls_ctx.ssl_ctx = SSL_CTX_new(TLS_server_method());

    const char* certificate_file = "server.crt";
    const char* private_key_file = "server.key";

    if (SSL_CTX_use_certificate_file(
        tls_ctx.ssl_ctx, certificate_file, SSL_FILETYPE_PEM) != 1)
    {
        printf("SSL_CTX_use_certificate_file failed\n");

        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(
        tls_ctx.ssl_ctx, private_key_file, SSL_FILETYPE_PEM) != 1)
    {
        printf("SSL_CTX_use_PrivateKey_file failed\n");

        return false;
    }

    self->server = co_tls_server_create(&local_net_addr, &tls_ctx);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    // callback
    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    // listen start
    co_tls_server_start(self->server, SOMAXCONN);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("listen %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->client_list);
    co_tls_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    co_tls_setup();

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}

#else

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    co_tls_setup();
    co_tls_cleanup();

    return 0;
}

#endif // CO_CAN_USE_TLS
