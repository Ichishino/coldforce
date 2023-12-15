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
    co_tcp_client_t* tcp_client;
    co_net_addr_t remote_net_addr;
    co_timer_t* retry_timer;

} app_st;

//---------------------------------------------------------------------------//
// tcp callback
//---------------------------------------------------------------------------//

void
app_on_tcp_receive(
    app_st* self,
    co_tcp_client_t* tcp_client
)
{
    (void)self;

    char buffer[1024];

    for (;;)
    {
        // receive data
        ssize_t size =
            co_tcp_receive(tcp_client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            return;
        }

        printf("tcp received: %zd bytes\n", (size_t)size);
    }
}

void
app_on_tcp_close(
    app_st* self,
    co_tcp_client_t* tcp_client
)
{
    printf("tcp closed\n");

    co_tcp_client_destroy(tcp_client);
    self->tcp_client = NULL;

    // quit app
    co_app_stop();
}

void
app_on_tcp_connect(
    app_st* self,
    co_tcp_client_t* tcp_client,
    int error_code
)
{
    if (error_code == 0)
    {
        printf("tcp connect success\n");

        // send
        const char* data = "hello";
        co_tcp_send(tcp_client, data, strlen(data));
    }
    else
    {
        printf("tcp connect failed\n");

        co_tcp_client_destroy(tcp_client);
        self->tcp_client = NULL;

        // start retry timer
        co_timer_start(self->retry_timer);
    }
}

//---------------------------------------------------------------------------//
// tcp connect
//---------------------------------------------------------------------------//

void
app_tcp_connect(
    app_st* self
)
{
    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(
        &local_net_addr, co_net_addr_get_family(&self->remote_net_addr));

    // create tcp client
    self->tcp_client = co_tcp_client_create(&local_net_addr);

    // callbacks
    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(self->tcp_client);
    callbacks->on_connect = (co_tcp_connect_fn)app_on_tcp_connect;
    callbacks->on_receive = (co_tcp_receive_fn)app_on_tcp_receive;
    callbacks->on_close = (co_tcp_close_fn)app_on_tcp_close;

    // start connect
    co_tcp_connect_start(self->tcp_client, &self->remote_net_addr);

    char remote_str[64];
    co_net_addr_to_string(
        &self->remote_net_addr, remote_str, sizeof(remote_str));
    printf("start connect to %s\n", remote_str);
}

//---------------------------------------------------------------------------//
// app callback
//---------------------------------------------------------------------------//

void
app_on_retry_timer(
    app_st* self,
    co_timer_t* timer
)
{
    (void)timer;

    // connect retry
    app_tcp_connect(self);
}

bool
app_on_create(
    app_st* self
)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count < 2 ||
        !co_net_addr_from_string(
            CO_NET_ADDR_FAMILY_IPV4, args->values[1],
            &self->remote_net_addr))
    {
        printf("<Usage>\n");
        printf("tcp_client <ip_address:port>\n");

        return false;
    }

    // create connect retry timer
    self->retry_timer = co_timer_create(
        5000, (co_timer_fn)app_on_retry_timer, false, 0);

    // start connect
    app_tcp_connect(self);

    return true;
}

void
app_on_destroy(
    app_st* self
)
{
    co_tcp_client_destroy(self->tcp_client);
    co_timer_destroy(self->retry_timer);
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

//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    app_st self = { 0 };

    return co_net_app_start(
        (co_app_t*)&self, "tcp-client-app",
        (co_app_create_fn)app_on_create,
        (co_app_destroy_fn)app_on_destroy,
        argc, argv);
}
