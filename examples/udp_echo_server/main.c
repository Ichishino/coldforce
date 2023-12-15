#include <coldforce.h>

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
    co_udp_t* udp;

} app_st;

//---------------------------------------------------------------------------//
// udp callback
//---------------------------------------------------------------------------//

void
app_on_udp_receive(
    app_st* self,
    co_udp_t* udp
)
{
    (void)self;

    for (;;)
    {
        co_net_addr_t remote_net_addr;
        char buffer[1024];

        // receive
        ssize_t size = co_udp_receive_from(
            udp, &remote_net_addr, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        char remote_str[64];
        co_net_addr_to_string(
            &remote_net_addr, remote_str, sizeof(remote_str));
        printf("received: %zd bytes from %s\n", (size_t)size, remote_str);

        // send (echo)
        co_udp_send_to(udp, &remote_net_addr, buffer, size);
    }
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
        printf("udp_echo_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    // create udp
    self->udp = co_udp_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(co_udp_get_socket(self->udp), true);

    // callbacks
    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(self->udp);
    callbacks->on_receive = (co_udp_receive_fn)app_on_udp_receive;

    // start receive
    co_udp_receive_start(self->udp);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("start receiver: %s\n", local_str);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_udp_destroy(self->udp);
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
        (co_app_t*)&self, "udp-echo-server-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
