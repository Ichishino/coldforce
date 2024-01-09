#include "test_tcp_mt_client.h"
#include "test_app.h"

static void
test_tcp_mt_server_on_req_close(
    test_tcp_mt_client_thread_st* self,
    const co_event_st* event
)
{
    (void)event;

    co_assert(!self->close);

    test_info("tcp mt client receive req-close");

    self->close = true;

    if (co_list_get_count(self->thread.clients) == 0)
    {
        co_thread_t* parent =
            co_thread_get_parent((co_thread_t*)self);

        co_thread_send_event(
            parent, TEST_EVENT_TCP_SERVER_RES_CLOSE, 0, 0);

        test_info("tcp mt client send res-close");
    }
}

static bool
test_tcp_mt_client_on_create(
    test_tcp_mt_client_thread_st* self
)
{
    self->close = false;

    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_TCP_SERVER_REQ_CLOSE,
        (co_event_fn)test_tcp_mt_server_on_req_close);

    // clients
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tcp_client_destroy;
    self->thread.clients = co_list_create(&list_ctx);

    co_net_thread_callbacks_st* callbacks =
        co_net_thread_get_callbacks((co_thread_t*)self);
    callbacks->on_tcp_accept = (co_tcp_accept_fn)test_tcp_server_on_accept;

    return true;
}

static void
test_tcp_mt_client_on_destroy(
    test_tcp_mt_client_thread_st* self
)
{
    if (co_list_get_count(self->thread.clients) != 0)
    {
        test_error(
            "Failed: test_tcp_mt_client_on_thread_destroy(%d, %d)",
            self->thread.ctx.family, self->thread.ctx.server_port);

        co_thread_set_exit_code(-1);
    }

    co_list_destroy(self->thread.clients);
}

test_tcp_mt_client_thread_st*
test_tcp_mt_client_thread_create(
    void
)
{
    test_tcp_mt_client_thread_st* thread =
        (test_tcp_mt_client_thread_st*)co_mem_alloc(
            sizeof(test_tcp_mt_client_thread_st));

    memset(thread, 0x00, sizeof(test_tcp_mt_client_thread_st));

    co_net_thread_setup(
        (co_thread_t*)thread, "test_tcp_mt_client_thread",
        (co_thread_create_fn)test_tcp_mt_client_on_create,
        (co_thread_destroy_fn)test_tcp_mt_client_on_destroy);

    if (!co_thread_start((co_thread_t*)thread))
    {
        test_error(
            "Failed: co_thread_start(test_tcp_mt_client_thread)");

        exit(-1);
    }

    return thread;
}

void
test_tcp_mt_client_thread_destroy(
    test_tcp_mt_client_thread_st* thread
)
{
    co_thread_stop((co_thread_t*)thread);
    co_thread_join((co_thread_t*)thread);
    co_net_thread_cleanup((co_thread_t*)thread);
    co_mem_free(thread);
}
