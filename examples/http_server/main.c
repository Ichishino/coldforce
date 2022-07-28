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
    co_tcp_server_t* server;

} my_app;

#define my_client_log(protocol, client, str) \
    do { \
        char remote_str[64]; \
        co_net_addr_to_string( \
            co_##protocol##_get_remote_net_addr(client), remote_str, sizeof(remote_str)); \
        printf("%s: %s\n", str, remote_str); \
    } while(0)

void on_my_http_stop_app_request(my_app* self, co_http_client_t* client)
{
    (void)self;

    my_client_log(http, client, "=== server stop ===");

    co_http_response_t* response = co_http_response_create_with(200, "OK");
    co_http_header_t* response_header = co_http_response_get_header(response);

    co_http_header_add_field(response_header, "Content-Type", "text/html");
    co_http_header_add_field(response_header, "Cache-Control", "no-store");

    const char* response_content =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Server Stop</title></head>\n"
        "<body>OK</body>\n"
        "</html>\n";
    uint32_t response_content_size = (uint32_t)strlen(response_content);

    co_http_header_set_content_length(response_header, response_content_size);

    co_http_send_response(client, response);
    co_http_send_data(client, response_content, response_content_size);

    // quit app
    co_app_stop();
}

void on_my_http_default_request(my_app* self, co_http_client_t* client, const co_http_request_t* request)
{
    (void)self;

    // request

    const co_http_header_t* request_header = co_http_request_get_const_header(request);

    size_t content_length = 0;
    co_http_header_get_content_length(request_header, &content_length);

    // response

    co_http_response_t* response = co_http_response_create_with(200, "OK");
    co_http_header_t* response_header = co_http_response_get_header(response);

    co_http_header_add_field(response_header, "Content-Type", "text/html");
    co_http_header_add_field(response_header, "Cache-Control", "no-store");

    char response_content[8192] = { 0 };
    char temp[1024];

    strcpy(response_content,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<title>Http2 Test</title>\n"
        "</head>\n"
        "<body>\n");

    strcat(response_content, "<h2>Client</h2>\n");

    const co_net_addr_t* remote_net_addr =
        co_http_get_remote_net_addr(client);
    char remote_str[256];
    co_net_addr_to_string(
        remote_net_addr, remote_str, sizeof(remote_str));

    sprintf(temp, "IP Address: %s<br>\n", remote_str);
    strcat(response_content, temp);

    strcat(response_content, "<h2>RequestHeader</h2>\n");

    strcat(response_content, co_http_request_get_method(request));
    strcat(response_content, " ");
    strcat(response_content, co_http_request_get_path(request));
    strcat(response_content, " ");
    strcat(response_content, co_http_request_get_version(request));
    strcat(response_content, "<br>\n");

    co_list_iterator_t* it =
        co_list_get_head_iterator(request_header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_next(request_header->field_list, &it);
        const co_http_header_field_t* field =
            (const co_http_header_field_t*)data->value;

        sprintf(temp, "%s: %s<br>\n", field->name, field->value);
        strcat(response_content, temp);
    }

    strcat(response_content, "<h2>Contents</h2>\n");
    sprintf(temp, "size: %zd<br><br>\n", content_length);
    strcat(response_content, temp);

    const char* data = (const char*)co_http_request_get_data(request);

    if (data != NULL)
    {
        strncat(response_content, data, content_length);
    }

    strcat(response_content, "</body>\n</html>\n");

    size_t response_content_size = strlen(response_content);

    co_http_header_set_content_length(response_header, response_content_size);

    // send reponse
    co_http_send_response(client, response);
    co_http_send_data(client, response_content, response_content_size);
}

void on_my_http_request(my_app* self, co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* unused, int error_code)
{
    (void)unused;

    my_client_log(http, client, "request");

    if (error_code == 0)
    {
        const co_http_url_st* url = co_http_request_get_url(request);

        if (strcmp(url->path, "/stop") == 0)
        {
            // server stop request
            // http://xxxxxxxxxxx/stop

            on_my_http_stop_app_request(self, client);
        }
        else
        {
            on_my_http_default_request(self, client, request);
        }
    }
    else
    {
        co_http_response_t* response =
            co_http_response_create_with(400, "Bad Request");

        co_http_send_response(client, response);
    }
    
    // close
    co_http_client_destroy(client);
}

void on_my_http_close(my_app* self, co_http_client_t* client)
{
    (void)self;

    my_client_log(http, client, "close");

    co_http_client_destroy(client);
}

void on_my_tcp_accept(my_app* self, co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    (void)tcp_server;

    my_client_log(tcp, tcp_client, "accept");

    co_tcp_accept((co_thread_t*)self, tcp_client);

    // create http client
    co_http_client_t* client = co_tcp_upgrade_to_http(tcp_client);

    // callback
    co_http_callbacks_st* callbacks = co_http_get_callbacks(client);
    callbacks->on_receive_finish = (co_http_receive_finish_fn)on_my_http_request;
    callbacks->on_close = (co_http_close_fn)on_my_http_close;
}

bool on_my_app_create(my_app* self)
{
    const co_args_st* args = co_app_get_args((co_app_t*)self);

    if (args->count <= 1)
    {
        printf("<Usage>\n");
        printf("http_server <port_number>\n");

        return false;
    }

    uint16_t port = (uint16_t)atoi(args->values[1]);

    // local address
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_tcp_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->server), true);

    // callback
    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->server);
    callbacks->on_accept = (co_tcp_accept_fn)on_my_tcp_accept;

    // listen start
    co_tcp_server_start(self->server, SOMAXCONN);

    printf("http://127.0.0.1:%d\n", port);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_tcp_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
//    co_http_log_set_level(CO_LOG_LEVEL_MAX);
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
