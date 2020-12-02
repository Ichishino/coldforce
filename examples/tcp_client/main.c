#include <coldforce/coldforce.h>
#include <coldforce/coldforce_net.h>

#include <stdio.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_tcp_client_t* client;
    co_timer_t* retry_timer;

} my_app;

co_tcp_client_t* create_my_tcp_client();

void on_my_tcp_receive(my_app* self, co_tcp_client_t* client)
{
    (void)self;

    char buffer[1024];

    // receive
    ssize_t size = co_tcp_receive(client, buffer, sizeof(buffer));

    printf("receive %zd bytes\n", (size_t)size);
}

void on_my_tcp_close(my_app* self, co_tcp_client_t* client)
{
    (void)client;

    printf("close\n");

    co_tcp_client_destroy(self->client);
    self->client = NULL;

    // quit app
    co_net_app_stop();
}

void on_my_tcp_connect(my_app* self, co_tcp_client_t* client, int error_code)
{
    if (error_code == 0)
    {
        printf("connect success\n");

        // send
        const char* data = "hello";
        co_tcp_send(client, data, strlen(data));
    }
    else
    {
        printf("connect failed\n");

        co_tcp_client_destroy(self->client);
        self->client = NULL;

        // start retry timer
        co_timer_start(self->retry_timer);
    }
}

void on_my_retry_timer(my_app* self, co_timer_t* timer)
{
    (void)timer;

    // recreate
    self->client = create_my_tcp_client();

    // connect retry
    co_tcp_connect_async(
        self->client, (co_tcp_connect_fn)on_my_tcp_connect);

    printf("retry connect\n");
}

co_tcp_client_t* create_my_tcp_client()
{
    const char* ip_address = "127.0.0.1";
    uint16_t port = 9000;

    // remote address
    co_net_addr_t remote_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_address(&remote_net_addr, ip_address);
    co_net_addr_set_port(&remote_net_addr, port);

    co_tcp_client_t* client = co_tcp_client_create(&remote_net_addr, NULL);

    co_tcp_set_receive_handler(client, (co_tcp_receive_fn)on_my_tcp_receive);
    co_tcp_set_close_handler(client, (co_tcp_close_fn)on_my_tcp_close);

    return client;
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    // connect retry timer
    self->retry_timer = co_timer_create(
        5000, (co_timer_fn)on_my_retry_timer, false, 0);

    // create my tcp client
    self->client = create_my_tcp_client();

    // connect async
    co_tcp_connect_async(
        self->client, (co_tcp_connect_fn)on_my_tcp_connect);

    char remote_str[64];
    co_get_remote_net_addr_as_string(self->client, remote_str);
    printf("connect to %s\n", remote_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_timer_destroy(self->retry_timer);
    co_tcp_client_destroy(self->client);
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
