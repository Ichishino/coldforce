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
    co_net_addr_t remote_net_addr;

} my_app;

void on_my_udp_receive(my_app* self, co_udp_t* udp)
{
    (void)self;

    for (;;)
    {
        char buffer[1024];

        // receive
        ssize_t size = co_udp_receive(udp, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        const co_net_addr_t* remote_net_addr =
            co_socket_get_remote_net_addr(co_udp_get_socket(self->udp));

        char remote_str[64];
        co_net_addr_to_string(remote_net_addr, remote_str, sizeof(remote_str));
        printf("receive %zd bytes from %s\n", (size_t)size, remote_str);
    }
}

void on_my_send_timer(my_app* self, co_timer_t* timer)
{
    // send
    const char* data = "hello";
    co_udp_send(self->udp, data, strlen(data) + 1);

    char remote_str[64];
    co_net_addr_to_string(&self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("send %s\n", remote_str);

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

    // remote address
    if (args->count < 2 ||
        !co_net_addr_from_string(
            CO_NET_ADDR_FAMILY_IPV4, args->values[1],
            &self->remote_net_addr))
    {
        printf("<Usage>\n");
        printf("udp_conn_client <ip_address:port>\n");

        return false;
    }

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    self->udp = co_udp_create(&local_net_addr);

    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(self->udp);
    callbacks->on_receive = (co_udp_receive_fn)on_my_udp_receive;

    // connect
    co_udp_connect(self->udp, &self->remote_net_addr);

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

    return co_net_app_start(
        (co_app_t*)&app, "my_app",
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);
}
