#include <coldforce.h>

#ifndef CO_OS_WIN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

//---------------------------------------------------------------------------//
// app object
//---------------------------------------------------------------------------//

typedef struct
{
    co_app_t base_app;

    // app data
    co_udp_server_t* udp_server;
    co_list_t* udp_clients;

} app_st;

#define app_get_remote_address(protocol, net_unit, buffer) \
    co_net_addr_to_string( \
        co_socket_get_remote_net_addr( \
            co_##protocol##_get_socket(net_unit)), \
        buffer, sizeof(buffer));

//---------------------------------------------------------------------------//
// udp callback
//---------------------------------------------------------------------------//

void
app_on_udp_receive(
    app_st* self,
    co_udp_t* udp_client
)
{
    for (;;)
    {
        char buffer[1024];

        // receive data
        ssize_t size =
            co_udp_receive(udp_client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        char remote_str[64];
        app_get_remote_address(udp, udp_client, remote_str);
        printf("received: %zd bytes from %s\n", (size_t)size, remote_str);

        // send
        co_udp_send(udp_client, buffer, size);
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
    (void)self;

    char remote_str[64];
    app_get_remote_address(udp, udp_client, remote_str);
    printf("udp accept %s\n", remote_str);

    // first received data
    const uint8_t* data;
    size_t data_size =
        co_udp_get_accept_data(udp_client, &data);

    // accept
    co_udp_accept((co_thread_t*)self, udp_client);

    // callbacks
    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(udp_client);
    callbacks->on_receive = (co_udp_receive_fn)app_on_udp_receive;
    callbacks->on_timer = (co_udp_timer_fn)app_on_udp_receive_timer;

    co_list_add_tail(self->udp_clients, udp_client);

    // send echo
    co_udp_send(udp_client, data, data_size);

    // start receive timer
    co_udp_create_timer(udp_client, 60*1000); // 1min
    co_udp_start_timer(udp_client);
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

    // create udp server
    self->udp_server = co_udp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_udp_server_get_socket(self->udp_server), true);

    // callbacks
    co_udp_server_callbacks_st* callbacks =
        co_udp_server_get_callbacks(self->udp_server);
    callbacks->on_accept = (co_udp_accept_fn)app_on_udp_accept;

    // start server
    co_udp_server_start(self->udp_server);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("start server %s\n", local_str);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_udp_server_destroy(self->udp_server);
    co_list_destroy(self->udp_clients);
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

//    co_udp_log_set_level(CO_LOG_LEVEL_MAX);

    // app instance
    app_st self = { 0 };

    // start app
    return co_net_app_start(
        (co_app_t*)&self, "udp-conn-server-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
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
