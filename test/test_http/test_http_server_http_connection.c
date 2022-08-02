#include "test_http_server_http_connection.h"
#include "test_http_server_ws_connection.h"

static void
http_server_on_http_request(
    http_server_thread* self,
    co_http_client_t* http_client,
    const co_http_request_t* request,
    const co_http_response_t* unused,
    int error_code
)
{
    (void)unused;

    if (error_code != 0)
    {
        co_list_remove(self->http_clients, http_client);

        return;
    }

    const co_http_url_st* url = co_http_request_get_url(request);

    co_string_map_t* query_map =
        co_http_url_query_parse(url->query, true);

    if (strcmp(url->path, "/") == 0 ||
        strcmp(url->path, "/index.html") == 0)
    {
        const char* data =
            "<html>"
            "<title>Coldforce Test (http/1.1)</title>"
            "<body> Hello !!! </body>"
            "</html>";
        size_t data_length = strlen(data);

        co_http_response_t* response =
            co_http_response_create_with(200, "OK");
        co_http_header_t* response_header =
            co_http_response_get_header(response);
        co_http_header_add_field(
            response_header, "Content-Type", "text/html");
        co_http_header_add_field(
            response_header, "Cache-Control", "no-store");
        co_http_header_set_content_length(
            response_header, data_length);

        const co_http_header_t* request_header =
            co_http_request_get_const_header(request);

        bool keep_alive =
            co_http_header_get_keep_alive(request_header);

        if (!keep_alive)
        {
            co_http_header_add_field(
                response_header, CO_HTTP_HEADER_CONNECTION, "close");
        }

        co_http_send_response(http_client, response);
        co_http_send_data(http_client, data, data_length);

        if (!keep_alive)
        {
            co_list_remove(self->http_clients, http_client);
        }
    }
    else if (strcmp(url->path, "/ws") == 0)
    {
        if (co_http_request_validate_ws_upgrade(request))
        {
            co_http_response_t* response =
                co_http_response_create_ws_upgrade(
                    request, NULL, NULL);
            co_http_connection_send_response(
                (co_http_connection_t*)http_client, response);
            co_http_response_destroy(response);

            co_list_iterator_t* it =
                co_list_find(self->http_clients, http_client);
            co_list_data_st* data =
                co_list_get(self->http_clients, it);
            data->value = NULL;
            co_list_remove_at(self->http_clients, it);

            add_ws_server_connection(self, http_client);
        }
        else
        {
            co_http_response_t* response =
                co_http_response_create_with(400, "Bad Rquest");
            co_http_send_response(http_client, response);

            co_list_remove(self->http_clients, http_client);
        }
    }
    else if (strcmp(url->path, "/stop") == 0)
    {
        bool is_basic_auth_ok = false;

        const co_http_header_t* request_header =
            co_http_request_get_const_header(request);

        const char* request_auth_str =
            co_http_header_get_field(
                request_header, CO_HTTP_HEADER_AUTHORIZATION);

        if (request_auth_str != NULL)
        {
            co_http_auth_t* request_auth =
                co_http_auth_create_request(request_auth_str);

            if (request_auth != NULL)
            {
                char* user = NULL;
                char* password = NULL;

                if (co_http_basic_auth_get_credentials(
                    request_auth, &user, &password))
                {
                    if (strcmp(user, "admin") == 0 &&
                        strcmp(password, "12345") == 0)
                    {
                        is_basic_auth_ok = true;
                    }

                    co_string_destroy(user);
                    co_string_destroy(password);
                }

                co_http_auth_destroy(request_auth);
            }
        }

        co_http_response_t* response = NULL;
        const char* data = NULL;

        if (is_basic_auth_ok)
        {
            response =
                co_http_response_create_with(200, "OK");

            data =
                "<html>"
                "<title>Coldforce Test (http/1.1)</title>"
                "<body> STOP OK </body>"
                "</html>";

            co_app_stop();
        }
        else
        {
            response =
                co_http_response_create_with(401, "Unauthorized");

            data =
                "<html>"
                "<title>Coldforce Test (http/1.1)</title>"
                "<body> Unauthorized </body>"
                "</html>";

            co_http_auth_t* response_auth =
                co_http_basic_auth_create_response("Coldforce Test Server");

            co_http_response_apply_auth(response,
                CO_HTTP_HEADER_WWW_AUTHENTICATE, response_auth);

            co_http_auth_destroy(response_auth);
        }

        co_http_header_t* response_header =
            co_http_response_get_header(response);

        size_t data_length = strlen(data);

        co_http_header_set_content_length(response_header, data_length);

        co_http_send_response(http_client, response);
        co_http_send_data(http_client, data, data_length);
    }
    else
    {
        co_http_response_t* response =
            co_http_response_create_with(404, "Not Found");

        co_http_header_t* response_header =
            co_http_response_get_header(response);

        co_http_header_add_field(
            response_header, "Content-Type", "text/html");

        const char* data =
            "<html>"
            "<title>Coldforce Test (http/1.1)</title>"
            "<body> Not Found </body>"
            "</html>";
        size_t data_length = strlen(data);

        co_http_header_set_content_length(response_header, data_length);

        co_http_send_response(http_client, response);
        co_http_send_data(http_client, data, data_length);

        co_list_remove(self->http_clients, http_client);
    }

    co_string_map_destroy(query_map);
}

static void
http_server_on_http_close(
    http_server_thread* self,
    co_http_client_t* client
)
{
    co_list_remove(self->http_clients, client);
}

void
add_http_server_connection(
    http_server_thread* self,
    co_tcp_client_t* tcp_client
)
{
    co_http_client_t* http_client =
        co_tcp_upgrade_to_http(tcp_client);

    co_http_callbacks_st* callbacks =
        co_http_get_callbacks(http_client);
    callbacks->on_receive_finish =
        (co_http_receive_finish_fn)http_server_on_http_request;
    callbacks->on_close =
        (co_http_close_fn)http_server_on_http_close;

    co_list_add_tail(self->http_clients, http_client);
}
