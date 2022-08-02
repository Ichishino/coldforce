#include "test_tcp_server_thread.h"

static void
on_tcp_receive(
    tcp_server_thread* self,
    co_tcp_client_t* client
)
{
    for (;;)
    {
        char buffer[1024];

        ssize_t size =
            co_tcp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            return;
        }

        co_tcp_send(client, buffer, (size_t)size);

        uint32_t r = co_random_range(100, 200);
        co_assert(r >= 100 && r <= 200);

        if (r == 100)
        {
            co_list_remove(self->clients, client);
        }
    }
}

static void
on_tcp_close(
    tcp_server_thread* self,
    co_tcp_client_t* client
)
{
    co_assert(co_list_get_count(self->clients) > 0);

    co_list_remove(self->clients, client);
}

static void
on_tcp_server_accept(
    tcp_server_thread* self,
    co_tcp_server_t* server,
    co_tcp_client_t* client
)
{
    (void)server;

    co_tcp_accept((co_thread_t*)self, client);

    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(client);
    callbacks->on_receive = (co_tcp_receive_fn)on_tcp_receive;
    callbacks->on_close = (co_tcp_close_fn)on_tcp_close;

    co_list_add_tail(self->clients, client);
}

static void
on_report_timer(
    tcp_server_thread* self,
    co_timer_t* timer
)
{
    (void)timer;

    co_log_info(
        LOG_CATEGORY_TEST_TCP_SERVER,
        "connection: %zu",
        co_list_get_count(self->clients));
}

static bool
on_tcp_server_thread_create(
    tcp_server_thread* self
)
{
    co_assert(self->port != 0);

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tcp_client_destroy;
    self->clients = co_list_create(&list_ctx);

    self->report_timer = co_timer_create(
        1000, (co_timer_fn)on_report_timer, true, NULL);
    co_timer_start(self->report_timer);

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, self->port);

    self->server = co_tcp_server_create(&local_net_addr);
    co_assert(self->server != NULL);

    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_tcp_server_accept;

    bool result = co_tcp_server_start(self->server, SOMAXCONN);
    co_assert(result);

    return result;
}

static void
on_tcp_server_thread_destroy(
    tcp_server_thread* self
)
{
    co_tcp_server_destroy(self->server);
    co_list_destroy(self->clients);
    co_timer_destroy(self->report_timer);
}

bool
tcp_server_thread_start(
    tcp_server_thread* thread
)
{
    co_net_thread_init(
        (co_thread_t*)thread,
        (co_thread_create_fn)on_tcp_server_thread_create,
        (co_thread_destroy_fn)on_tcp_server_thread_destroy);

    return co_thread_start((co_thread_t*)thread);
}
