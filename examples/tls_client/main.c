#include <coldforce/coldforce_tls.h>

#include <stdio.h>
#include <string.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_tcp_client_t* client;
    co_timer_t* retry_timer;

} my_app;

void my_connect(my_app* self);

void on_my_tls_receive(my_app* self, co_tcp_client_t* client)
{
    (void)self;

    for (;;)
    {
        char buffer[1024];

        ssize_t size = co_tls_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        printf("receive %zd bytes\n", (size_t)size);
    }
}

void on_my_tls_close(my_app* self, co_tcp_client_t* client)
{
    (void)client;

    printf("close\n");

    co_tls_client_destroy(self->client);
    self->client = NULL;

    // quit app
    co_net_app_stop();
}

void on_my_tls_connect(my_app* self, co_tcp_client_t* client, int error_code)
{
    if (error_code == 0)
    {
        printf("connect success\n");

        // send
        const char* data = "hello";
        co_tls_send(client, data, strlen(data));
    }
    else
    {
        printf("connect failed\n");

        co_tls_client_destroy(self->client);
        self->client = NULL;

        // start retry timer
        co_timer_start(self->retry_timer);
    }
}

void on_my_retry_timer(my_app* self, co_timer_t* timer)
{
    (void)timer;

    // connect retry
    my_connect(self);
}

void my_connect(my_app* self)
{
    const char* ip_address = "127.0.0.1";
    uint16_t port = 9443;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->client = co_tls_client_create(&local_net_addr, NULL);

    co_tls_set_receive_handler(self->client, (co_tcp_receive_fn)on_my_tls_receive);
    co_tls_set_close_handler(self->client, (co_tcp_close_fn)on_my_tls_close);

    // remote address
    co_net_addr_t remote_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_address(&remote_net_addr, ip_address);
    co_net_addr_set_port(&remote_net_addr, port);

    // connect
    co_tls_connect(
        self->client, &remote_net_addr, (co_tcp_connect_fn)on_my_tls_connect);

    char remote_str[64];
    co_net_addr_get_as_string(&remote_net_addr, remote_str);
    printf("connect to %s\n", remote_str);
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    // connect retry timer
    self->retry_timer = co_timer_create(
        5000, (co_timer_fn)on_my_retry_timer, false, 0);

    // connect
    my_connect(self);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_timer_destroy(self->retry_timer);
    co_tls_client_destroy(self->client);
}

int main(int argc, char* argv[])
{
    co_tls_setup();

    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
