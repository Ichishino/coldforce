#define _CRT_SECURE_NO_WARNINGS
#include <coldforce/coldforce_http2.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http2_client_t* client;

} my_app;

bool on_my_push_request(my_app* self, co_http2_client_t* client, const co_http2_stream_t* stream,
    co_http2_message_t* push_message)
{
    (void)self;
    (void)client;

    const co_http2_message_t* send_message = co_http2_stream_get_send_message(stream);

    const co_http2_header_t* send_header = co_http2_message_get_const_header(send_message);
    const co_http2_header_t* push_header = co_http2_message_get_const_header(push_message);

    const char* send_path = co_http2_header_get_path(send_header);
    const char* push_path = co_http2_header_get_path(push_header);

    printf("send path: %s\n", send_path);
    printf("push path: %s\n", push_path);
    
    char* file_name =
        co_http_url_create_file_name(co_http2_header_get_url(push_header));

    char local_save_path[1024];
    sprintf(local_save_path, "./dl/%s", file_name);
    printf("local_save_path: %s\n", local_save_path);

    // save to file (optional)
    co_http2_message_set_content_file_path(push_message, local_save_path);

    co_http_url_destroy_string(file_name);

    return true;
}

void on_my_push_response(my_app* self, co_http2_client_t* client, co_http2_stream_t* stream,
    const co_http2_message_t* receive_message, int error_code)
{
    (void)self;

    printf("push response: %d\n", error_code);

    if (error_code == 0)
    {
        const co_http2_message_t* request_message = co_http2_stream_get_send_message(stream);
        const co_http2_header_t* request_header = co_http2_message_get_const_header(request_message);
        const char* server_push_path = co_http2_header_get_path(request_header);

        const co_http2_header_t* response_header = co_http2_message_get_const_header(receive_message);

        uint16_t status_code = co_http2_header_get_status_code(response_header);
        size_t content_size = co_http2_message_get_content_size(receive_message);

        printf("status code: %d\n", status_code);
        printf("server push path: %s\n", server_push_path);
        printf("content size: %zu\n", content_size);
    }

    if (!co_http2_client_is_running(client))
    {
        // quit app
        co_net_app_stop();
    }
}

void on_my_receive(my_app* self, co_http2_client_t* client, co_http2_stream_t* stream,
    const co_http2_message_t* receive_message, int error_code)
{
    (void)self;

    printf("receive: %d\n", error_code);

    if (error_code == 0)
    {
        const co_http2_message_t* request_message = co_http2_stream_get_send_message(stream);
        const co_http2_header_t* request_header = co_http2_message_get_const_header(request_message);
        const co_http2_header_t* response_header = co_http2_message_get_const_header(receive_message);

        const char* request_path = co_http2_header_get_path(request_header);
        uint16_t status_code = co_http2_header_get_status_code(response_header);
        size_t content_size = co_http2_message_get_content_size(receive_message);

        printf("request path: %s\n", request_path);
        printf("status code: %d\n", status_code);
        printf("content size: %zu\n", content_size);

        const co_http2_header_t* header = co_http2_message_get_const_header(receive_message);
        const char* content_type = co_http2_header_get_field(header, "content-type");

        if (content_type != NULL)
        {
            printf("content type: %s\n", content_type);
        }

        const void* content = co_http2_message_get_content(receive_message);

        printf("%s\n", (const char*)content);
    }

    if (!co_http2_client_is_running(client))
    {
        // quit app
        co_net_app_stop();
    }
}

void on_my_close(my_app* self, co_http2_client_t* client, int error_code)
{
    (void)self;
    (void)client;

    printf("close(%d)\n", error_code);

    // quit app
    co_net_app_stop();
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    const char* base_url = "https://www.example.com";
    const char* file_path = "/index.html";

    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->client = co_http2_client_create(base_url, &local_net_addr, NULL);

    co_http2_set_message_handler(self->client, (co_http2_message_fn)on_my_receive);
    co_http2_set_close_handler(self->client, (co_http2_close_fn)on_my_close);

    // server push
    co_http2_set_server_push_request_handler(self->client, (co_http2_push_request_fn)on_my_push_request);
    co_http2_set_server_push_response_handler(self->client, (co_http2_message_fn)on_my_push_response);

    co_http2_message_t* message = co_http2_message_create_request("GET", file_path);

    // set header
    co_http2_header_t* header = co_http2_message_get_header(message);
    co_http2_header_add_field(header, "accept", "text/html");

    // send request
    co_http2_send_request(self->client, message);

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
