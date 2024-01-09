#include "test_tcp_server.h"
#include "test_tcp_server_comm.h"
#include "test_app.h"

static void
test_tcp_server_on_req_close(
    test_tcp_server_thread_st* self,
    const co_event_st* event
)
{
    (void)event;

    co_assert(!self->close);

    test_info("tcp server receive req-close");

    self->close = true;

    if (co_list_get_count(self->thread.clients) == 0)
    {
        co_thread_t* parent =
            co_thread_get_parent((co_thread_t*)self);

        co_thread_send_event(
            parent, TEST_EVENT_TCP_SERVER_RES_CLOSE, 0, 0);

        test_info("tcp server send res-close");
    }
}

static bool
test_tcp_server_on_thread_create(
    test_tcp_server_thread_st* self
)
{
    self->close = false;

    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_TCP_SERVER_REQ_CLOSE,
        (co_event_fn)test_tcp_server_on_req_close);

    // clients

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tcp_client_destroy;
    self->thread.clients = co_list_create(&list_ctx);

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

    // server

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
        (co_tcp_accept_fn)test_tcp_server_on_accept;

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
    test_info("tcp server start: %s", msg);

    return true;
}

static void
test_tcp_server_on_thread_destroy(
    test_tcp_server_thread_st* self
)
{
    co_tcp_server_destroy(self->tcp_server);

    if (co_list_get_count(self->thread.clients) != 0)
    {
        test_error(
            "Failed: test_tcp_server_on_thread_destroy(%d, %d)",
            self->thread.ctx.family, self->thread.ctx.server_port);

        co_thread_set_exit_code(-1);
    }

    co_list_destroy(self->thread.clients);
}

void
test_tcp_server_thread_start(
    test_tcp_server_thread_st* test_tcp_server_thread
)
{
    co_net_thread_setup(
        (co_thread_t*)test_tcp_server_thread, "test_tcp_server_thread",
        (co_thread_create_fn)test_tcp_server_on_thread_create,
        (co_thread_destroy_fn)test_tcp_server_on_thread_destroy);

    if (!co_thread_start((co_thread_t*)test_tcp_server_thread))
    {
        test_error("Failed: co_thread_start(test_tcp_server_thread)");

        exit(-1);
    }
}

void
test_tcp_server_thread_stop(
    test_tcp_server_thread_st* test_tcp_server_thread
)
{
    co_thread_stop((co_thread_t*)test_tcp_server_thread);
    co_thread_join((co_thread_t*)test_tcp_server_thread);
    co_net_thread_cleanup((co_thread_t*)test_tcp_server_thread);
}
