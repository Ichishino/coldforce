#include "my_client_thread.h"
#include "my_server_app.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool add_client(my_client_thread* self)
{
    co_mutex_lock(self->mutex);

    bool result = ((co_list_get_count(self->client_list) +
        self->reserve) < MAX_CLIENTS_PER_THREAD);

    if (result)
    {
        ++self->reserve;
    }

    co_mutex_unlock(self->mutex);

    return result;
}

void on_my_tcp_receive(my_client_thread* self, co_tcp_client_t* client)
{
    (void)self;

    for (;;)
    {
        char buffer[8192];

        // receive
        ssize_t size = co_tcp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            break;
        }

        char remote_str[64];
        co_net_addr_to_string(
            co_tcp_get_remote_net_addr(client), remote_str, sizeof(remote_str));
        printf("receive %zd bytes from %s\n", (size_t)size, remote_str);

        // send
        co_tcp_send(client, buffer, (size_t)size);
    }
}

void on_my_tcp_close(my_client_thread* self, co_tcp_client_t* client)
{
    char remote_str[64];
    co_net_addr_to_string(
        co_tcp_get_remote_net_addr(client), remote_str, sizeof(remote_str));
    printf("close %s\n", remote_str);

    co_mutex_lock(self->mutex);
    co_list_remove(self->client_list, client);
    co_mutex_unlock(self->mutex);
}

void on_my_client_tcp_accept(my_client_thread* self, co_tcp_server_t* unused, co_tcp_client_t* client)
{
    (void)unused;

    // accept
    co_tcp_accept((co_thread_t*)self, client);

    // callback
    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(client);
    callbacks->on_receive = (co_tcp_receive_fn)on_my_tcp_receive;
    callbacks->on_close = (co_tcp_close_fn)on_my_tcp_close;

    co_mutex_lock(self->mutex);
    co_list_add_tail(self->client_list, client);
    --self->reserve;
    co_mutex_unlock(self->mutex);

    char remote_str[64];
    co_net_addr_to_string(
        co_tcp_get_remote_net_addr(client), remote_str, sizeof(remote_str));
    printf("accept %s\n", remote_str);
}

bool on_my_client_thread_create(my_client_thread* self, void* param)
{
    (void)param;

    self->reserve = 0;
    self->mutex = co_mutex_create();

    // client list
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tcp_client_destroy; // auto destroy
    self->client_list = co_list_create(&list_ctx);

    co_net_thread_callbacks_st* callbacks =
        co_net_thread_get_callbacks((co_thread_t*)self);
    callbacks->on_tcp_accept = (co_tcp_accept_fn)on_my_client_tcp_accept;

    return true;
}

void on_my_client_thread_destroy(my_client_thread* self)
{
    co_list_destroy(self->client_list);
    co_mutex_destroy(self->mutex);
}

void init_my_client_thread(my_client_thread* thread)
{
    co_net_thread_setup(
        (co_thread_t*)thread, "my_client_thread",
        (co_thread_create_fn)on_my_client_thread_create,
        (co_thread_destroy_fn)on_my_client_thread_destroy);
}
