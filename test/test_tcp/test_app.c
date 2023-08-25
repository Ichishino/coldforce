#include "test_app.h"

static bool
on_test_app_create(
    test_app* self
)
{
    self->tcp_client.client_count = 100;
    self->tcp_client.server_address = "127.0.0.1";
    self->tcp_client.server_port = 9000;
    tcp_client_thread_start(&self->tcp_client);

    self->tcp_server.port = 9000;
    tcp_server_thread_start(&self->tcp_server);

    return true;
}

static void
on_test_app_destroy(
    test_app* self
)
{
    co_thread_stop((co_thread_t*)&self->tcp_client);
    co_thread_stop((co_thread_t*)&self->tcp_server);

    co_thread_join((co_thread_t*)&self->tcp_client);
    co_thread_join((co_thread_t*)&self->tcp_server);

    co_net_thread_cleanup((co_thread_t*)&self->tcp_client);
    co_net_thread_cleanup((co_thread_t*)&self->tcp_server);
}

int
test_app_run(
    int argc,
    char** argv
)
{
    test_app app = { 0 };

    return co_net_app_start(
        (co_app_t*)&app,
        (co_app_create_fn)on_test_app_create,
        (co_app_destroy_fn)on_test_app_destroy,
        argc, argv);
}
