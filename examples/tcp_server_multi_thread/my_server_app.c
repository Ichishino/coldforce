#include "my_server_app.h"

#include <stdio.h>

void on_my_tcp_accept(my_server_app* self, co_tcp_server_t* server, co_tcp_client_t* client)
{
    (void)server;

    // round robin

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t index = self->thread_index + i;

        if (index >= THREAD_COUNT)
        {
            index -= THREAD_COUNT;
        }

        if (add_client(&self->client_thread[index]))
        {
            // accept
            co_tcp_accept((co_thread_t*)&self->client_thread[index], client);

            self->thread_index = index + 1;

            if (self->thread_index >= THREAD_COUNT)
            {
                self->thread_index = 0;
            }

            return;
        }
    }

    co_tcp_client_destroy(client);
}

bool on_my_server_app_create(my_server_app* self, const co_arg_st* arg)
{
    (void)arg;

    printf("%d threads\n", THREAD_COUNT);
    printf("max %d clients\n", THREAD_COUNT * MAX_CLIENTS_PER_THREAD);

    self->thread_index = 0;

    // create my client threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        init_my_client_thread(&self->client_thread[i]);
        co_net_thread_start((co_thread_t*)&self->client_thread[i], 0);
    }

    uint16_t port = 9000;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT_IPV4;
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr((co_socket_t*)self->server, true);

    // listen start
    co_tcp_server_start(self->server,
        (co_tcp_accept_fn)on_my_tcp_accept, SOMAXCONN);

    char local_str[64];
    co_net_addr_get_as_string(&local_net_addr, local_str);
    printf("listen %s\n", local_str);

    return true;
}

void on_my_server_app_destroy(my_server_app* self)
{
    // stop and cleanup my client threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        co_net_thread_stop((co_thread_t*)&self->client_thread[i]);
    }
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        co_net_thread_wait((co_thread_t*)&self->client_thread[i]);
        co_net_thread_cleanup((co_thread_t*)&self->client_thread[i]);
    }

    co_tcp_server_destroy(self->server);

    printf("\napp has finished\n");
}

void init_my_server_app(my_server_app* server_app)
{
    co_net_app_init(
        (co_app_t*)server_app,
        (co_create_fn)on_my_server_app_create,
        (co_destroy_fn)on_my_server_app_destroy);
}
