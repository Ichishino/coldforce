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
    co_timer_t* send_timer;
    int send_counter;
    const char* peer_ip_address;
    uint16_t peer_port;

} my_app;

void on_my_send_timer(my_app* self, co_timer_t* timer)
{
    // remote address
    co_net_addr_t remote_net_addr = { 0 };
    co_net_addr_set_address(&remote_net_addr, self->peer_ip_address);
    co_net_addr_set_port(&remote_net_addr, self->peer_port);

    // send
    const char* data = "hello";
    co_udp_send(self->udp, &remote_net_addr, data, strlen(data) + 1);

    char remote_str[64];
    co_net_addr_to_string(&remote_net_addr, remote_str, sizeof(remote_str));
    printf("send to %s\n", remote_str);

    self->send_counter++;

    // quit app if send 10 times
    if (self->send_counter == 10)
    {
        co_timer_stop(timer);
        co_app_stop();
    }
}

bool on_my_app_create(my_app* self)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count < 3)
    {
        printf("<Usage>\n");
        printf("udp_sender <peer_ip_address> <port_number>\n");

        return false;
    }

    self->peer_ip_address = args->values[1];
    self->peer_port = (uint16_t)atoi(args->values[2]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
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
//    co_udp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);

    // run
    int exit_code = co_app_run((co_app_t*)&app);

    co_net_app_cleanup((co_app_t*)&app);

    return exit_code;
}
