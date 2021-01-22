#include <coldforce/coldforce_tls.h>

#include <stdio.h>

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

        co_list_remove(self->client_list, (uintptr_t)client);
    }
}

void on_my_tcp_receive(my_app* self, co_tcp_client_t* client)
{
    (void)self;

    co_byte_array_t* byte_array = co_byte_array_create();

    // receive
    ssize_t data_size = co_tls_tcp_receive_all(client, byte_array);

    printf("receive %zd bytes\n", (size_t)data_size);

    if (data_size > 0)
    {
        unsigned char* data = co_byte_array_get_ptr(byte_array, 0);

        // send (echo)
        co_tls_tcp_send(client, data, data_size);
    }

    co_byte_array_destroy(byte_array);
}

void on_my_tcp_close(my_app* self, co_tcp_client_t* client)
{
    char remote_str[64];
    co_get_remote_net_addr_as_string(client, remote_str);
    printf("close %s\n", remote_str);

    co_list_remove(self->client_list, (uintptr_t)client);
}

void on_my_tcp_accept(my_app* self, co_tcp_server_t* server, co_tcp_client_t* client)
{
    (void)server;

    // accept
    co_tcp_accept((co_thread_t*)self, client);

    co_tls_tcp_set_receive_handler(client, (co_tcp_receive_fn)on_my_tcp_receive);
    co_tls_tcp_set_close_handler(client, (co_tcp_close_fn)on_my_tcp_close);

    // TLS handshake
    co_tls_tcp_start_handshake(
        client, (co_tls_tcp_handshake_fn)on_my_tls_handshake);

    co_list_add_tail(self->client_list, (uintptr_t)client);

    char remote_str[64];
    co_get_remote_net_addr_as_string(client, remote_str);
    printf("accept %s\n", remote_str);
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_item_free_fn)co_tls_tcp_client_destroy; // auto destroy
    self->client_list = co_list_create(&list_ctx);

    uint16_t port = 9443;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);
    
    // TLS setting (openssl)
    co_tls_ctx_st tls_ctx;
    tls_ctx.ssl_ctx = SSL_CTX_new(TLS_server_method());
    SSL_CTX_use_certificate_file(tls_ctx.ssl_ctx, "server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(tls_ctx.ssl_ctx, "server.key", SSL_FILETYPE_PEM);

    self->server = co_tls_tcp_server_create(&local_net_addr, &tls_ctx);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    // listen start
    co_tls_tcp_server_start(self->server,
        (co_tcp_accept_fn)on_my_tcp_accept, SOMAXCONN);

    char local_str[64];
    co_net_addr_get_as_string(&local_net_addr, local_str);
    printf("listen %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->client_list);
    co_tls_tcp_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
    co_tls_setup();

    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
