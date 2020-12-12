#include <coldforce/coldforce_net.h>

#include <stdio.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_udp_t* udp;
    co_timer_t* send_timer;
    int send_counter;

} my_app;

void on_my_send_timer(my_app* self, co_timer_t* timer)
{
    const char* ip_address = "127.0.0.1";
    uint16_t port = 9001;

    // remote address
    co_net_addr_t remote_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_address(&remote_net_addr, ip_address);
    co_net_addr_set_port(&remote_net_addr, port);

    // send
    const char* data = "hello";
    co_udp_send(self->udp, &remote_net_addr, data, strlen(data));

    char remote_str[64];
    co_net_addr_get_as_string(&remote_net_addr, remote_str);
    printf("send to %s\n", remote_str);

    self->send_counter++;

    // quit app if send 10 times
    if (self->send_counter == 10)
    {
        co_timer_stop(timer);
        co_net_app_stop();
    }
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->udp = co_udp_create(&local_net_addr);

    // socket option
    co_socket_option_set_send_buffer(
        co_udp_get_socket(self->udp), 10000);

    // send timer
    self->send_counter = 0;
    self->send_timer = co_timer_create(1000, (co_timer_fn)on_my_send_timer, true, 0);
    co_timer_start(self->send_timer);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_timer_destroy(self->send_timer);
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
