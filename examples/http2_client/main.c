#include <coldforce/coldforce_http2.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http2_client_t* http2_client;

} my_app;

void on_my_http2_response(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* response_header, const co_http2_data_st* response_data,
    int error_code)
{
    (void)self;

    printf("=== response(%d) ===\n", error_code);

    if (error_code == 0)
    {
        const co_http2_header_t* request_header = co_http2_stream_get_send_header(stream);

        const char* request_path = co_http2_header_get_path(request_header);
        uint16_t status_code = co_http2_header_get_status_code(response_header);

        printf("request path: %s\n", request_path);
        printf("status code: %d\n", status_code);
        printf("content size: %zu\n", response_data->size);

        const char* content_type = co_http2_header_get_field(response_header, "content-type");

        if (content_type != NULL)
        {
            printf("content type: %s\n", content_type);
        }

        const void* content = response_data->ptr;

        printf("%s\n", (const char*)content);
    }

    if (!co_http2_is_running(http2_client))
    {
        // quit app
        co_net_app_stop();
    }
}

void on_my_http2_close(my_app* self, co_http2_client_t* http2_client, int error_code)
{
    (void)self;
    (void)http2_client;

    printf("=== close(%d) ===\n", error_code);

    // quit app
    co_net_app_stop();
}

void on_my_http2_upgrade(my_app* self, co_http_client_t* http1_client, int error_code)
{
    printf("=== upgrade(%d) ===\n", error_code);

    if (error_code == 0)
    {
        // upgrade

        self->http2_client = co_http2_client_upgrade(http1_client);

        co_http2_set_message_handler(self->http2_client, (co_http2_message_fn)on_my_http2_response);
        co_http2_set_close_handler(self->http2_client, (co_http2_close_fn)on_my_http2_close);

        // send request

        co_http2_header_t* header = co_http2_header_create_request("GET", "/");
        co_http2_header_add_field(header, "accept", "text/html");

        co_http2_stream_t* stream = co_http2_create_stream(self->http2_client); // new stream per request

        co_http2_stream_send_header(stream, true, header);
    }
    else
    {
        printf("*** upgrade failed ***\n");

        co_http_client_destroy(http1_client);

        // quit app
        co_net_app_stop();
    }
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)self;
    (void)arg;

    self->http2_client = NULL;

    const char* base_url = "http://127.0.0.1:9080";

    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    co_http_client_t* http1_client =
        co_http_client_create(base_url, &local_net_addr, NULL);

    // send upgrade request
    co_http2_send_upgrade_request(
        http1_client, "/", NULL, 0,
        (co_http_upgrade_response_fn)on_my_http2_upgrade);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http2_client_destroy(self->http2_client);
}

int main(int argc, char* argv[])
{
    co_tls_setup();

    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_create_fn)on_my_app_create,
        (co_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
