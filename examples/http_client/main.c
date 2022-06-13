#include <coldforce/coldforce_http.h>

// my app object
typedef struct
{
    co_app_t base_app;

    // my app data
    co_http_client_t* client;

} my_app;

void on_my_response(my_app* self, co_http_client_t* client,
    const co_http_response_t* response, int error_code)
{
    (void)self;
    (void)client;

    if (error_code == 0)
    {
        uint16_t status_code = co_http_response_get_status_code(response);
        size_t content_size = co_http_response_get_content_size(response);

        printf("status code: %d\n", status_code);
        printf("content size: %zu\n", content_size);

        const co_http_header_t* header = co_http_response_get_const_header(response);
        const char* content_type = co_http_header_get_field(header, "Content-Type");

        if (content_type != NULL)
        {
            printf("content type: %s\n", content_type);
        }

        const void* content = co_http_response_get_content(response);

        printf("%s\n", (const char*)content);
    }
    else
    {
        printf("error: %d\n", error_code);
    }

    if (!co_http_is_running(self->client))
    {
        // quit app
        co_net_app_stop();
    }
}

void on_my_connect(my_app* self, co_http_client_t* client, int error_code)
{
    if (error_code == 0)
    {
        co_http_set_receive_handler(self->client, (co_http_receive_fn)on_my_response);

        // you can send http requests in parallel

        // GET request
        {
            co_http_request_t* request = co_http_request_create_with("GET", "/index.html");

            // set header
            co_http_header_t* header = co_http_request_get_header(request);
            co_http_header_add_field(header, "Accept", "text/html");

            // send request
            co_http_send_request(self->client, request);
        }

        // POST request
        {
            co_http_request_t* request = co_http_request_create_with("POST", "/index.html");

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
    }
    else
    {
        co_http_client_destroy(client);
        self->client = NULL;

        // quit app
        co_net_app_stop();
    }
}

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    (void)arg;

#ifdef CO_CAN_USE_TLS
    const char* base_url = "https://www.example.com";
#else
    const char* base_url = "http://www.example.com";
#endif

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_ADDRESS_FAMILY_IPV4);

    self->client = co_http_client_create(base_url, &local_net_addr, NULL);

    if (self->client == NULL)
    {
        return false;
    }

    // connect start
    co_http_connect(self->client, (co_http_connect_fn)on_my_connect);

    return true;
}

void on_my_app_destroy(my_app* self)
{
    co_http_client_destroy(self->client);
}

int main(int argc, char* argv[])
{
    co_tls_setup();

    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    co_tls_cleanup();

    return exit_code;
}
