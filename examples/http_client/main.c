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
    co_http_client_t* client;
    co_url_st* url;
    const char* save_file_path;
    FILE* fp;

} my_app;

bool on_my_receive_start(my_app* self, co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* response)
{
    (void)client;

    const char* path = co_http_request_get_path(request);
    printf("request path: %s\n", path);

    const co_http_header_t* response_header = co_http_response_get_const_header(response);
    size_t length = 0;
    co_http_header_get_content_length(response_header, &length);

    printf("content length: %zd\n", length);

    // file open
    self->fp = fopen(self->save_file_path, "wb");

    if (self->fp == NULL)
    {
        // error
        return false;
    }

    return true;
}

bool on_my_receive_data(my_app* self, co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* response,
    const uint8_t* data_block, size_t data_block_size)
{
    (void)client;
    (void)request;
    (void)response;

    // write data to file
    if (fwrite(data_block, data_block_size, 1, self->fp) != 1)
    {
        // cancel receiving
        return false;
    }

    return true;
}

void on_my_receive_finish(my_app* self, co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* response,
    int error_code)
{
    (void)self;
    (void)client;
    (void)request;

    if (error_code == 0)
    {
        uint16_t status_code = co_http_response_get_status_code(response);
        size_t data_size = co_http_response_get_data_size(response);

        printf("status code: %d\n", status_code);
        printf("data size: %zu\n", data_size);

        const co_http_header_t* header = co_http_response_get_const_header(response);
        const char* content_type = co_http_header_get_field(header, "Content-Type");

        if (content_type != NULL)
        {
            printf("content type: %s\n", content_type);
        }

        const void* data = co_http_response_get_data(response);

        if (data != NULL)
        {
            printf("%s\n", (const char*)data);
        }
    }
    else
    {
        printf("error: %d\n", error_code);
    }

    if (self->fp != NULL)
    {
        fclose(self->fp);
        self->fp = NULL;
    }

    if (!co_http_is_running(self->client))
    {
        // quit app
        co_app_stop();
    }
}

void on_my_connect(my_app* self, co_http_client_t* client, int error_code)
{
    if (error_code == 0)
    {
        // GET request
        {
            co_http_request_t* request = co_http_request_create("GET", self->url->path_and_query);

            // set header
            co_http_header_t* header = co_http_request_get_header(request);
            co_http_header_add_field(header, "Accept", "text/html");

            // send request
            co_http_send_request(self->client, request);
        }
/*
        // POST request
        {
            co_http_request_t* request = co_http_request_create("POST", self->url->path_and_query);

            // set header
            co_http_header_t* header = co_http_request_get_header(request);
            co_http_header_add_field(header, "Accept", "text/html");

            // content
            const char* data = "{\"data\":\"hello\"}";
            size_t data_len = strlen(data);
            co_http_header_add_field(header, "Content-Type", "application/json");
            co_http_header_set_content_length(header, data_len);

            // send request
            co_http_send_request(self->client, request);
            co_http_send_data(self->client, data, data_len);
        }
*/
    }
    else
    {
        printf("connect error: %d\n", error_code);

        co_http_client_destroy(client);
        self->client = NULL;

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
        printf("http_client <url> [save_file_path]\n");

        return false;
    }

    self->url = co_url_create(args->values[1]);

    if (args->count >= 3)
    {
        self->save_file_path = args->values[2];
    }

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    self->client = co_http_client_create(self->url->origin, &local_net_addr, NULL);

    if (self->client == NULL)
    {
        printf("error: faild to resolve hostname or OpenSSL is not installed\n");

        return false;
    }

    // callback
    co_http_callbacks_st* callbacks = co_http_get_callbacks(self->client);
    callbacks->on_connect = (co_http_connect_fn)on_my_connect;
    callbacks->on_receive_finish = (co_http_receive_finish_fn)on_my_receive_finish;

    if (self->save_file_path != NULL)
    {
        callbacks->on_receive_start = (co_http_receive_start_fn)on_my_receive_start;
        callbacks->on_receive_data = (co_http_receive_data_fn)on_my_receive_data;
    }

    // connect start
    co_http_connect(self->client);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http_client_destroy(self->client);
    co_url_destroy(self->url);

    if (self->fp != NULL)
    {
        fclose(self->fp);
    }
}

int main(int argc, char* argv[])
{
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_MAX);

    my_app app = { 0 };

    co_net_app_setup(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy,
        argc, argv);

    // run
    int exit_code = co_app_run((co_app_t*)&app);

    co_net_app_cleanup((co_app_t*)&app);

    return exit_code;
}
