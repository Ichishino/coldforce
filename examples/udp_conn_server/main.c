#include <coldforce.h>

#ifndef CO_OS_WIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_udp_server_t* udp_server;
    co_list_t* udp_clients;

} my_app;

void on_my_udp_receive(my_app* self, co_udp_t* udp_client)
{
    for (;;)
    {
        char buffer[1024];

        // receive
        ssize_t size = co_udp_receive(udp_client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        const co_net_addr_t* remote_net_addr =
            co_socket_get_remote_net_addr(co_udp_get_socket(udp_client));

        char remote_str[64];
        co_net_addr_to_string(remote_net_addr, remote_str, sizeof(remote_str));

        if (memcmp(buffer, "close", 5) == 0)
        {
            printf("close %s\n", remote_str);

            co_list_remove(self->udp_clients, udp_client);

            break;
        }

        printf("client receive %zd bytes from %s\n", (size_t)size, remote_str);

        co_udp_send(udp_client, buffer, size);
    }
}

void on_my_udp_accept(my_app* self, co_udp_server_t* udp_server, co_udp_t* udp_client)
{
    (void)self;

    // first received data
    const uint8_t* data;
    size_t data_size =
        co_udp_get_accept_data(udp_client, &data);

    // accept
    co_udp_accept((co_thread_t*)self, udp_client);

    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(udp_client);
    callbacks->on_receive = (co_udp_receive_fn)on_my_udp_receive;

    co_list_add_tail(self->udp_clients, udp_client);

    // echo
    co_udp_send(udp_client, data, data_size);
}

bool on_my_app_create(my_app* self)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count <= 1)
    {
        printf("<Usage>\n");
        printf("udp_conn_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_udp_destroy; // auto destroy
    self->udp_clients = co_list_create(&list_ctx);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->udp_server = co_udp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(co_udp_server_get_socket(self->udp_server), true);

    // callback
    co_udp_server_callbacks_st* callbacks = co_udp_server_get_callbacks(self->udp_server);
    callbacks->on_accept = (co_udp_accept_fn)on_my_udp_accept;

    // server start
    co_udp_server_start(self->udp_server);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("server %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_udp_server_destroy(self->udp_server);
    co_list_destroy(self->udp_clients);
}

int main(int argc, char* argv[])
{
//    co_udp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    return co_net_app_start(
        (co_app_t*)&app, "my_app",
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);
}

#else

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    printf("UDP connection server is not supported on Windows.\n");

    return 0;
}

#endif // !CO_OS_WIN
