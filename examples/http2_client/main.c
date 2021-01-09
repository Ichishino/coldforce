#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <coldforce/coldforce_http2.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http2_client_t* client;

} my_app;

bool on_my_push_request(
    my_app* self, co_http2_client_t* http2_client,
    const co_http2_stream_t* push_request_stream, co_http2_stream_t* push_response_stream,
    const co_http2_header_t* push_request_header)
{
    (void)self;
    (void)http2_client;

    printf("=== server push request ===\n");

    const co_http2_header_t* send_header = co_http2_stream_get_send_header(push_request_stream);

    const char* send_path = co_http2_header_get_path(send_header);
    const char* push_path = co_http2_header_get_path(push_request_header);

    printf("send path: %s\n", send_path);
    printf("push path: %s\n", push_path);

    char* file_name =
        co_http_url_create_file_name(co_http2_header_get_path_url(push_request_header));

    char local_save_path[1024];
    sprintf(local_save_path, "./%s", file_name);
    printf("local_save_path: %s\n", local_save_path);

    // save to file (optional)
    co_http2_stream_set_save_file_path(push_response_stream, local_save_path);

    co_http_url_destroy_string(file_name);

    return true;
}

void on_my_push_response(
    my_app* self, co_http2_client_t* http2_client, const co_http2_stream_t* push_response_stream,
    const co_http2_header_t* push_response_header, const co_http2_data_st* push_response_data,
    int error_code)
{
    (void)self;

    printf("=== server push response(%d) ===\n", error_code);

    if (error_code == 0)
    {
        const co_http2_header_t* push_request_header = co_http2_stream_get_send_header(push_response_stream);
        const char* server_push_path = co_http2_header_get_path(push_request_header);

        uint16_t status_code = co_http2_header_get_status_code(push_response_header);

        printf("status code: %d\n", status_code);
        printf("server push path: %s\n", server_push_path);
        printf("content size: %zu\n", push_response_data->size);
    }

    if (!co_http2_is_running(http2_client))
    {
        // quit app
        co_net_app_stop();
    }
}

void on_my_response(
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

        if (response_data->ptr != NULL)
        {
            printf("%s\n", (const char*)response_data->ptr);
        }
    }

    if (!co_http2_is_running(http2_client))
    {
        // quit app
        co_net_app_stop();
    }
}

void on_my_close(my_app* self, co_http2_client_t* client, int error_code)
{
    (void)self;
    (void)client;

    printf("=== close(%d) ===\n", error_code);

    // quit app
    co_net_app_stop();
}

void on_my_connect(my_app* self, co_http2_client_t* client, int error_code)
{
    (void)self;
    (void)client;

    printf("=== connect(%d) ===\n", error_code);

    if (error_code == 0)
    {
        co_http2_header_t* header = co_http2_header_create_request("GET", "/");
        co_http2_header_add_field(header, "accept", "text/html");

        // new stream per request
        co_http2_stream_t* stream = co_http2_create_stream(client);

        // send request
        co_http2_stream_send_header(stream, true, header);
    }
    else
    {
        printf("*** connect failed ***\n");

        // quit app
        co_net_app_stop();
    }
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    const char* base_url = "https://127.0.0.1:9443";

    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->client = co_http2_client_create(base_url, &local_net_addr, NULL);

    co_http2_setting_param_st params[1];
    params[0].identifier = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
    params[0].value = CO_HTTP2_SETTING_MAX_WINDOW_SIZE;
    co_http2_init_settings(self->client, params, 1);

    // set callback handler
    co_http2_set_message_handler(self->client, (co_http2_message_fn)on_my_response);
    co_http2_set_close_handler(self->client, (co_http2_close_fn)on_my_close);
    co_http2_set_server_push_request_handler(self->client, (co_http2_push_request_fn)on_my_push_request);
    co_http2_set_server_push_response_handler(self->client, (co_http2_push_response_fn)on_my_push_response);

    // connect 
    co_http2_connect(self->client, (co_http2_connect_fn)on_my_connect);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http2_client_destroy(self->client);
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
