#include <coldforce/coldforce.h>
#include <coldforce/coldforce_net.h>

#include <stdio.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_tcp_server_t* server;
    co_list_t* client_list;

} my_app;

void on_my_tcp_receive(my_app* self, co_tcp_client_t* client)
{
    (void)self;

    char buffer[1024];

    // receive
    ssize_t size = co_tcp_receive(client, buffer, sizeof(buffer));

    char remote_str[64];
    co_get_remote_net_addr_as_string(client, remote_str);
    printf("receive %zd bytes from %s\n", (size_t)size, remote_str);

    // send
    co_tcp_send(client, buffer, (size_t)size);
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

    co_tcp_set_receive_handler(client, (co_tcp_receive_fn)on_my_tcp_receive);
    co_tcp_set_close_handler(client, (co_tcp_close_fn)on_my_tcp_close);

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
    list_ctx.free_value = (co_free_fn)co_tcp_client_destroy; // auto destroy
    self->client_list = co_list_create(&list_ctx);

    uint16_t port = 9000;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr((co_socket_t*)self->server, true);

    // listen start
    co_tcp_server_start(self->server,
        (co_tcp_accept_fn)on_my_tcp_accept, SOMAXCONN);

    char local_str[64];
    co_net_addr_get_as_string(&local_net_addr, local_str);
    printf("listen %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_list_destroy(self->client_list);
    co_tcp_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_create_fn)on_my_app_create,
        (co_destroy_fn)on_my_app_destroy);

    // app start
    return co_net_app_start((co_app_t*)&app, argc, argv);
}
