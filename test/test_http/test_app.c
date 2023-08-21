#include "test_app.h"

static bool
on_test_app_create(
    test_app* self
)
{
    self->http_server.port = 8443;
    self->http_server.certificate_file = "server.crt";
    self->http_server.private_key_file = "server.key";
 
    if (!http_server_thread_start(&self->http_server))
    {
        co_app_set_exit_code(-100);

        return false;
    }

    self->ws_http2_client.url = co_url_create("https://127.0.0.1:8443/ws-over-http2");
    ws_http2_client_thread_start(&self->ws_http2_client);

    return true;
}

static void
on_test_app_destroy(
    test_app* self
)
{
    co_thread_stop((co_thread_t*)&self->http_server);
    co_thread_stop((co_thread_t*)&self->ws_http2_client);

    co_thread_join((co_thread_t*)&self->http_server);
    co_thread_join((co_thread_t*)&self->ws_http2_client);

    co_net_thread_cleanup((co_thread_t*)&self->http_server);
    co_net_thread_cleanup((co_thread_t*)&self->ws_http2_client);
}

int
test_app_run(
    int argc,
    char** argv
)
{
    test_app app = { 0 };

    co_net_app_setup(
        (co_app_t*)&app,
        (co_app_create_fn)on_test_app_create,
        (co_app_destroy_fn)on_test_app_destroy,
        argc, argv);

    int exit_code = co_app_run((co_app_t*)&app);

    co_net_app_cleanup((co_app_t*)&app);

    return exit_code;
}
