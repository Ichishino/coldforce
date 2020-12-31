#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <coldforce/coldforce_http.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http_server_t* server;

} my_app;

#define my_client_log(client, str) \
    do { \
        char remote_str[64]; \
        co_net_addr_get_as_string( \
            co_http_get_remote_net_addr(client), remote_str); \
        printf("%s: %s\n", str, remote_str); \
    } while(0)

void on_my_http_stop_app_request(my_app* self, co_http_client_t* client)
{
    (void)self;

    printf("server stop\n");

    co_http_response_t* response = co_http_response_create_with(200, "OK");
    co_http_header_t* response_header = co_http_response_get_header(response);

    co_http_header_add_field(response_header, "Content-Type", "text/html");

    const char* str =
        "<html>"
        "<head><title>Server Stop</title></head>"
        "<body>OK</body>"
        "</html>";

    co_http_response_set_content(response, str, strlen(str));

    co_http_send_response(client, response);

    // quit app
    co_net_app_stop();
}

void on_my_http_default_request(my_app* self, co_http_client_t* client, const co_http_request_t* request)
{
    (void)self;

    // request

    const char* method = co_http_request_get_method(request);

    const co_http_url_st* url = co_http_request_get_url(request);
    const char* path = url->path;
    const char* query = (url->query != NULL) ? url->query : "";

    const co_http_header_t* request_header = co_http_request_get_const_header(request);
    const char* host = co_http_header_get_field(request_header, CO_HTTP_HEADER_HOST);

    size_t content_length = 0;
    co_http_header_get_content_length(request_header, &content_length);

    // response

    co_http_response_t* response = co_http_response_create_with(200, "OK");
    co_http_header_t* response_header = co_http_response_get_header(response);

    co_http_header_add_field(response_header, "Content-Type", "text/html");

    char response_content[8192];
    sprintf(response_content,
        "<html>\n"
        "<head><title>Http Test</title></head>\n"
        "<body>\n"
        "method: %s<br>\n"
        "request url: %s<br>\n"
        "query: %s<br>\n"
        "host: %s<br>\n"
        "content length: %zu<br>\n"
        "</body>\n"
        "</html>\n",
        method, path, query, host, content_length);

    size_t response_content_size = strlen(response_content);

    co_http_header_set_content_length(response_header, response_content_size);

    // send reponse
    co_http_send_response(client, response);
    co_http_send_data(client, response_content, response_content_size);
}

void on_my_http_request(my_app* self, co_http_client_t* client, const co_http_request_t* request, int error_code)
{
    my_client_log(client, "request");

    if (error_code == 0)
    {
        co_http_request_print_header(request);

        const co_http_url_st* url = co_http_request_get_url(request);

        if (strcmp(url->path, "/stop") == 0)
        {
            // server stop command
            // http://127.0.0.1:9080/stop

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

    my_client_log(client, "close");

    co_http_client_destroy(client);
}

void on_my_http_accept(my_app* self, co_http_server_t* server, co_http_client_t* client)
{
    my_client_log(client, "accept");

    // accept
    co_http_accept(server, (co_thread_t*)self, client);

    // set callback
    co_http_set_request_handler(client, (co_http_request_fn)on_my_http_request);
    co_http_set_close_handler(client, (co_http_close_fn)on_my_http_close);
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

    uint16_t port = 9080;

    // local address
    co_net_addr_t local_net_addr = CO_NET_ADDR_INIT;
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);
    co_net_addr_set_port(&local_net_addr, port);

    self->server = co_http_server_create(&local_net_addr);

    // socket option
    co_socket_option_set_reuse_addr(
        co_http_server_get_socket(self->server), true);

    // listen start
    co_http_server_start(self->server,
        (co_http_accept_fn)on_my_http_accept, SOMAXCONN);

    char local_str[64];
    co_net_addr_get_as_string(&local_net_addr, local_str);
    printf("listen %s\n", local_str);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http_server_destroy(self->server);
}

int main(int argc, char* argv[])
{
    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_create_fn)on_my_app_create,
        (co_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    return exit_code;
}
