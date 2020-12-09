#include <coldforce/coldforce_net.h>

#include <stdio.h>

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
        co_net_addr_get_as_string(&remote_net_addr, remote_str);
        printf("receive %zd bytes from %s\n", (size_t)size, remote_str);

        // send (echo)
        co_udp_send(udp, &remote_net_addr, buffer, size);
    }
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    uint16_t port = 9001;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT_IPV4;
    co_net_addr_set_port(&local_net_addr, port);

    self->udp = co_udp_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr((co_socket_t*)self->udp, true);
#ifdef _WIN32
    co_win_udp_set_receive_buffer_size(self->udp, 10000);
#else
    co_socket_option_set_receive_buffer((co_socket_t*)self->udp, 10000);
#endif

    // receive start
    co_udp_receive_start(self->udp, (co_udp_receive_fn)on_my_udp_receive);

    char local_str[64];
    co_net_addr_get_as_string(&local_net_addr, local_str);
    printf("bind %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_udp_destroy(self->udp);
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
