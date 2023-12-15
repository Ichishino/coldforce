#include "tcp_client_thread.h"
#include "tcp_server_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool
tcp_client_thread_add(
    tcp_client_thread_st* self
)
{
    co_mutex_lock(self->mutex);

    bool result = ((co_list_get_count(self->tcp_clients) +
        self->reserve) < MAX_CLIENTS_PER_THREAD);

    if (result)
    {
        ++self->reserve;
    }

    co_mutex_unlock(self->mutex);

    return result;
}

void
tcp_client_thread_on_tcp_receive(
    tcp_client_thread_st* self,
    co_tcp_client_t* tcp_client
)
{
    (void)self;

    for (;;)
    {
        char buffer[8192];

        // receive
        ssize_t size =
            co_tcp_receive(tcp_client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        char remote_str[64];
        app_get_remote_address(tcp, tcp_client, remote_str);
        printf("tcp received: %zd bytes from %s\n", (size_t)size, remote_str);

        // send
        co_tcp_send(tcp_client, buffer, (size_t)size);
    }
}

void
tcp_client_thread_on_tcp_close(
    tcp_client_thread_st* self,
    co_tcp_client_t* tcp_client
)
{
    char remote_str[64];
    app_get_remote_address(tcp, tcp_client, remote_str);
    printf("tcp closed: %s\n", remote_str);

    co_mutex_lock(self->mutex);
    co_list_remove(self->tcp_clients, tcp_client);
    co_mutex_unlock(self->mutex);
}

void
tcp_client_thread_on_tcp_accept(
    tcp_client_thread_st* self,
    co_tcp_server_t* unused,
    co_tcp_client_t* tcp_client
)
{
    (void)unused;

    // accept
    co_tcp_accept((co_thread_t*)self, tcp_client);

    // callbacks
    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(tcp_client);
    callbacks->on_receive = (co_tcp_receive_fn)tcp_client_thread_on_tcp_receive;
    callbacks->on_close = (co_tcp_close_fn)tcp_client_thread_on_tcp_close;

    co_mutex_lock(self->mutex);
    co_list_add_tail(self->tcp_clients, tcp_client);
    --self->reserve;
    co_mutex_unlock(self->mutex);

    char remote_str[64];
    app_get_remote_address(tcp, tcp_client, remote_str);
    printf("accept: %s\n", remote_str);
}

bool
tcp_client_thread_on_create(
    tcp_client_thread_st* self
)
{
    self->reserve = 0;
    self->mutex = co_mutex_create();

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value =
        (co_item_destroy_fn)co_tcp_client_destroy; // auto destroy
    self->tcp_clients = co_list_create(&list_ctx);

    co_net_thread_callbacks_st* callbacks =
        co_net_thread_get_callbacks((co_thread_t*)self);
    callbacks->on_tcp_accept =
        (co_tcp_accept_fn)tcp_client_thread_on_tcp_accept;

    return true;
}

void
tcp_client_thread_on_destroy(
    tcp_client_thread_st* self
)
{
    co_list_destroy(self->tcp_clients);
    co_mutex_destroy(self->mutex);
}

//---------------------------------------------------------------------------//
// start tcp client thread
//---------------------------------------------------------------------------//

void
tcp_client_thread_start(
    tcp_client_thread_st* thread
)
{
    co_net_thread_setup(
        (co_thread_t*)thread, "tcp-client-thread",
        (co_thread_create_fn)tcp_client_thread_on_create,
        (co_thread_destroy_fn)tcp_client_thread_on_destroy);

    co_thread_start((co_thread_t*)thread);
}
