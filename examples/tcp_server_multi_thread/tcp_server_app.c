#include "tcp_server_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
tcp_server_app_on_tcp_accept(
    tcp_server_app_st* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    (void)tcp_server;

    // round robin

    for (size_t i = 0; i < THREAD_COUNT; ++i)
    {
        size_t index = self->thread_index + i;

        if (index >= THREAD_COUNT)
        {
            index -= THREAD_COUNT;
        }

        if (tcp_client_thread_add(
            &self->tcp_client_thread[index]))
        {
            // accept
            co_tcp_accept(
                (co_thread_t*)&self->tcp_client_thread[index],
                tcp_client);

            self->thread_index = index + 1;

            if (self->thread_index >= THREAD_COUNT)
            {
                self->thread_index = 0;
            }

            return;
        }
    }

    co_tcp_client_destroy(tcp_client);
}

bool
tcp_server_app_on_create(
    tcp_server_app_st* self
)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count <= 1)
    {
        printf("<Usage>\n");
        printf("tcp_server_multi_thread <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    printf("%d threads\n", THREAD_COUNT);
    printf("max %d clients\n", THREAD_COUNT * MAX_CLIENTS_PER_THREAD);

    self->thread_index = 0;

    // start tcp client threads
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        tcp_client_thread_start(&self->tcp_client_thread[i]);
    }

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    // create tcp server
    self->tcp_server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->tcp_server), true);

    // callbacks
    co_tcp_server_callbacks_st* callbacks =
        co_tcp_server_get_callbacks(self->tcp_server);
    callbacks->on_accept =
        (co_tcp_accept_fn)tcp_server_app_on_tcp_accept;

    // start listen
    co_tcp_server_start(self->tcp_server, SOMAXCONN);

    char local_str[64];
    co_net_addr_to_string(&local_net_addr, local_str, sizeof(local_str));
    printf("start server: %s\n", local_str);

    return true;
}

void
tcp_server_app_on_destroy(
    tcp_server_app_st* self
)
{
    // stop and cleanup tcp client threads

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        co_thread_stop((co_thread_t*)&self->tcp_client_thread[i]);
    }
    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        co_thread_join((co_thread_t*)&self->tcp_client_thread[i]);
        co_net_thread_cleanup((co_thread_t*)&self->tcp_client_thread[i]);
    }

    co_tcp_server_destroy(self->tcp_server);

    printf("\napp has finished\n");
}

//---------------------------------------------------------------------------//
// start tcp server app
//---------------------------------------------------------------------------//

int
tcp_server_app_run(
    int argc,
    char** argv
)
{
    tcp_server_app_st server_app = { 0 };

    return co_net_app_start(
        (co_app_t*)&server_app, "tcp-server-app",
        (co_app_create_fn)tcp_server_app_on_create,
        (co_app_destroy_fn)tcp_server_app_on_destroy,
        argc, argv);
}
