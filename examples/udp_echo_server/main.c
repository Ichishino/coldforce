#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_udp_t* udp;

} my_app;

void on_my_udp_receive(my_app* self, co_udp_t* udp)
{
    (void)self;

    for (;;)
    {
        co_net_addr_t remote_net_addr;
        char buffer[1024];

        // receive
        ssize_t size = co_udp_receive(
            udp, &remote_net_addr, &buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        char remote_str[64];
        co_net_addr_to_string(&remote_net_addr, remote_str, sizeof(remote_str));
        printf("receive %zd bytes from %s\n", (size_t)size, remote_str);

        // send (echo)
        co_udp_send(udp, &remote_net_addr, buffer, size);
    }
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    if (arg->argc <= 1)
    {
        printf("<Usage>\n");
        printf("udp_echo_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(arg->argv[1]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->udp = co_udp_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(co_udp_get_socket(self->udp), true);
#ifdef _WIN32
    co_win_udp_set_receive_buffer_size(self->udp, 65535);
#else
    co_socket_option_set_receive_buffer(co_udp_get_socket(self->udp), 65535);
#endif

    // callback
    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(self->udp);
    callbacks->on_receive = (co_udp_receive_fn)on_my_udp_receive;

    // receive start
    co_udp_receive_start(self->udp);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("bind %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_udp_destroy(self->udp);
}

int main(int argc, char* argv[])
{
//    co_udp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    return co_net_app_start((co_app_t*)&app, argc, argv);
}
