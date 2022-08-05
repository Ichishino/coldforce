#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <coldforce.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http2_client_t* client;
    char* base_url;
    char* path;
    char* save_file_path;
    FILE* fp;

} my_app;

bool on_my_receive_start(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* response_header)
{
    (void)http2_client;
    (void)stream;
    (void)response_header;

    self->fp = fopen(self->save_file_path, "wb");

    return true;
}

bool on_my_receive_data(
    my_app* self, co_http2_client_t* http2_client, co_http2_stream_t* stream,
    const co_http2_header_t* response_header, const co_http2_data_st* data_block)
{
    (void)http2_client;
    (void)stream;
    (void)response_header;

    fwrite(data_block->ptr, data_block->size, 1, self->fp);

    return true;
}

void on_my_receive_finish(
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

        const char* content_type = co_http2_header_get_field(response_header, "content-type");

        if (content_type != NULL)
        {
            printf("content type: %s\n", content_type);
        }

        printf("content size: %zu\n", response_data->size);

        if (response_data->ptr != NULL)
        {
            printf("%s\n", (const char*)response_data->ptr);
        }
    }

    if (self->fp != NULL)
    {
        fclose(self->fp);
        self->fp = NULL;
    }

    if (!co_http2_is_running(http2_client))
    {
        // quit app
        co_app_stop();
    }
}

void on_my_close(my_app* self, co_http2_client_t* client, int error_code)
{
    (void)self;
    (void)client;

    printf("=== close(%d) ===\n", error_code);

    // quit app
    co_app_stop();
}

void on_my_connect(my_app* self, co_http2_client_t* client, int error_code)
{
    (void)self;
    (void)client;

    printf("=== connect(%d) ===\n", error_code);

    if (error_code == 0)
    {
        co_http2_header_t* header = co_http2_header_create_request("GET", self->path);
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
        co_app_stop();
    }
}

bool on_my_app_create(my_app* self)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count < 2)
    {
        printf("<Usage>\n");
        printf("http2_client <url> [save_file_path]\n");

        return false;
    }

    co_url_st* url = co_url_create(args->values[1]);

    self->base_url = co_url_create_base_url(url);
    self->path = co_url_create_path_and_query(url);

    co_url_destroy(url);

    if (args->count >= 3)
    {
        self->save_file_path = args->values[2];
    }

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->client = co_http2_client_create(self->base_url, &local_net_addr, NULL);

    if (self->client == NULL)
    {
        printf("error: faild to resolve hostname or OpenSSL is not installed\n");

        return false;
    }

    co_http2_setting_param_st params[1];
    params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
    params[0].value = CO_HTTP2_SETTING_MAX_WINDOW_SIZE;
    co_http2_init_settings(self->client, params, 1);

    // callback
    co_http2_callbacks_st* callback = co_http2_get_callbacks(self->client);
    callback->on_connect = (co_http2_connect_fn)on_my_connect;
    callback->on_receive_finish = (co_http2_receive_finish_fn)on_my_receive_finish;
    callback->on_close = (co_http2_close_fn)on_my_close;

    if (self->save_file_path != NULL)
    {
        callback->on_receive_start = (co_http2_receive_start_fn)on_my_receive_start;
        callback->on_receive_data = (co_http2_receive_data_fn)on_my_receive_data;
    }

    // connect 
    co_http2_connect(self->client);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http2_client_destroy(self->client);

    co_string_destroy(self->base_url);
    co_string_destroy(self->path);

    if (self->fp != NULL)
    {
        fclose(self->fp);
        self->fp = NULL;
    }
}

int main(int argc, char* argv[])
{
//    co_http2_log_set_level(CO_LOG_LEVEL_MAX);
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);

    // run
    int exit_code = co_app_run((co_app_t*)&app);

    co_net_app_cleanup((co_app_t*)&app);

    return exit_code;
}
