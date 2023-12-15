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
    co_timer_t* send_timer;
    int send_counter;
    co_net_addr_t remote_net_addr;

} app_st;

//---------------------------------------------------------------------------//
// timer callback
//---------------------------------------------------------------------------//

void
app_on_send_timer(
    app_st* self,
    co_timer_t* timer
)
{
    // send
    const char* data = "hello";
    co_udp_send_to(self->udp,
        &self->remote_net_addr, data, strlen(data) + 1);

    char remote_str[64];
    co_net_addr_to_string(
        &self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("send to %s\n", remote_str);

    self->send_counter++;

    // quit app if send 10 times
    if (self->send_counter == 10)
    {
        co_timer_stop(timer);
        co_app_stop();
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

    // remote address
    if (args->count < 2 ||
        !co_net_addr_from_string(
            CO_NET_ADDR_FAMILY_IPV4, args->values[1],
            &self->remote_net_addr))
    {
        printf("<Usage>\n");
        printf("udp_sender <ip_address:port>\n");

        return false;
    }

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    self->udp = co_udp_create(&local_net_addr);

    // send timer
    self->send_counter = 0;
    self->send_timer = co_timer_create(
        1000, (co_timer_fn)app_on_send_timer, true, 0);
    co_timer_start(self->send_timer);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_timer_destroy(self->send_timer);
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
        (co_app_t*)&self, "udp-sender-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
