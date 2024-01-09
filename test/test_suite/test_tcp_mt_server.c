#include "test_tcp_mt_server.h"
#include "test_tcp_mt_client.h"

#include "test_app.h"

static void
test_tcp_mt_server_on_res_close(
    test_tcp_mt_server_thread_st* self,
    const co_event_st* event
)
{
    (void)event;

    test_info("tcp mt server receive res-close");

    self->stopped_thread_count++;

    if (self->stopped_thread_count == self->thread_count)
    {
        co_thread_t* parent =
            co_thread_get_parent((co_thread_t*)self);

        co_thread_send_event(
            parent, TEST_EVENT_TCP_SERVER_RES_CLOSE, 0, 0);

        test_info("tcp mt server send res-close");
    }
}

static void
test_tcp_mt_server_on_req_close(
    test_tcp_mt_server_thread_st* self,
    const co_event_st* event
)
{
    (void)event;

    co_assert(!self->close);

    test_info("tcp mt server receive req-close");

    self->close = true;

    co_list_iterator_t* it =
        co_list_get_head_iterator(self->thread.clients);

    while (it != NULL)
    {
        co_list_data_st* data =
            co_list_get_next(self->thread.clients, &it);

        co_thread_send_event(
            (co_thread_t*)data->value,
            TEST_EVENT_TCP_SERVER_REQ_CLOSE,
            0, 0);
    }
}

void
test_tcp_mt_server_on_accept(
    test_tcp_mt_server_thread_st* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    (void)tcp_server;

    co_assert(self->thread_it != NULL);

    co_list_data_st* data =
        co_list_get_next(self->thread.clients, &self->thread_it);

    co_assert(data != NULL);

    co_tcp_accept((co_thread_t*)data->value, tcp_client);

    if (self->thread_it == NULL)
    {
        self->thread_it =
            co_list_get_head_iterator(self->thread.clients);
    }
}

static bool
test_tcp_mt_server_on_create(
    test_tcp_mt_server_thread_st* self
)
{
    self->close = false;

    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_TCP_SERVER_REQ_CLOSE,
        (co_event_fn)test_tcp_mt_server_on_req_close);

    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_TCP_SERVER_RES_CLOSE,
        (co_event_fn)test_tcp_mt_server_on_res_close);

    // client threads

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value =
        (co_item_destroy_fn)test_tcp_mt_client_thread_destroy;
    self->thread.clients = co_list_create(&list_ctx);

    for (size_t index = 0; index < self->thread_count; index++)
    {
        co_list_add_tail(
            self->thread.clients,
            test_tcp_mt_client_thread_create());
    }

    self->thread_it =
        co_list_get_head_iterator(self->thread.clients);

    // server

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, self->thread.ctx.family);

    if (self->thread.ctx.family == CO_NET_ADDR_FAMILY_UNIX)
    {
        co_net_addr_set_unix_path(
            &local_net_addr, self->thread.ctx.server_address);
    }
    else
    {
        co_net_addr_set_port(
            &local_net_addr, self->thread.ctx.server_port);
    }

    self->tcp_server = co_tcp_server_create(&local_net_addr);

    if (self->tcp_server == NULL)
    {
        test_error(
            "Failed: co_tcp_server_create(%d, %d)",
            self->thread.ctx.family, self->thread.ctx.server_port);

        exit(-1);
    }

    co_tcp_server_callbacks_st* callbacks =
        co_tcp_server_get_callbacks(self->tcp_server);
    callbacks->on_accept =
        (co_tcp_accept_fn)test_tcp_mt_server_on_accept;

    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->tcp_server), true);

    // server start

    if (!co_tcp_server_start(self->tcp_server, SOMAXCONN))
    {
        test_error(
            "Failed: co_tcp_server_start(%d, %d)",
            self->thread.ctx.family, self->thread.ctx.server_port);

        exit(-1);
    }

    char msg[256];
    co_net_addr_to_string(&local_net_addr, msg, sizeof(msg));
    test_info("tcp mt server start: %s", msg);

    return true;
}

static void
test_tcp_mt_server_on_destroy(
    test_tcp_mt_server_thread_st* self
)
{
    co_tcp_server_destroy(self->tcp_server);
    co_list_destroy(self->thread.clients);
}

void
test_tcp_mt_server_thread_start(
    test_tcp_mt_server_thread_st* thread
)
{
    co_net_thread_setup(
        (co_thread_t*)thread, "test_tcp_mt_server_thread",
        (co_thread_create_fn)test_tcp_mt_server_on_create,
        (co_thread_destroy_fn)test_tcp_mt_server_on_destroy);

    if (!co_thread_start((co_thread_t*)thread))
    {
        test_error("Failed: co_thread_start(test_tcp_mt_server_thread)");

        exit(-1);
    }
}

void
test_tcp_mt_server_thread_stop(
    test_tcp_mt_server_thread_st* thread
)
{
    co_thread_stop((co_thread_t*)thread);
    co_thread_join((co_thread_t*)thread);
    co_net_thread_cleanup((co_thread_t*)thread);
}
