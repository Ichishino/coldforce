#include <coldforce/coldforce_net.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    if (size <= 0)
    {
        return;
    }

    char remote_str[64];
    co_net_addr_to_string(
        co_tcp_get_remote_net_addr(client), remote_str, sizeof(remote_str));
    printf("receive %zd bytes from %s\n", (size_t)size, remote_str);

    // send
    co_tcp_send(client, buffer, (size_t)size);
}

void on_my_tcp_close(my_app* self, co_tcp_client_t* client)
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

    // callback
    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(client);
    callbacks->on_receive = (co_tcp_receive_fn)on_my_tcp_receive;
    callbacks->on_close = (co_tcp_close_fn)on_my_tcp_close;

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
        printf("tcp_echo_server port_number\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(arg->argv[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tcp_client_destroy; // auto destroy
    self->client_list = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    // callback
    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    // listen start
    co_tcp_server_start(self->server, SOMAXCONN);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
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
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    return co_net_app_start((co_app_t*)&app, argc, argv);
}
