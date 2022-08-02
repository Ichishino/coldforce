#include "test_http_server_http2_connection.h"

//---------------------------------------------------------------------------//
// websocket over http2
//---------------------------------------------------------------------------//

static void
http_server_on_ws_http2_receive_frame(
    http_server_thread* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_ws_frame_t* frame
)
{
    (void)self;
    (void)http2_client;

    bool fin = co_ws_frame_get_fin(frame);
    uint8_t opcode = co_ws_frame_get_opcode(frame);
    size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
    const uint8_t* data = co_ws_frame_get_payload_data(frame);

    switch (opcode)
    {
    case CO_WS_OPCODE_TEXT:
    case CO_WS_OPCODE_BINARY:
    case CO_WS_OPCODE_CONTINUATION:
    {
        // echo
        co_http2_stream_send_ws_frame(stream,
            fin, opcode, data, (size_t)data_size);

        // close stream
        // co_http2_stream_send_ws_close(stream, 0, NULL);

        break;
    }
    case CO_WS_OPCODE_CLOSE:
        // peer stream closed
    default:
    {
        co_http2_stream_ws_default_handler(stream, frame);

        break;
    }
    }
}

//---------------------------------------------------------------------------//
// http2
//---------------------------------------------------------------------------//

static void
http_server_on_http2_request(
    http_server_thread* self,
    co_http2_client_t* http2_client,
    co_http2_stream_t* stream,
    const co_http2_header_t* request_header,
    const co_http2_data_st* request_data,
    int error_code
)
{
    if (error_code != 0)
    {
        co_list_remove(self->http2_clients, http2_client);

        return;
    }

    // websocket over http2
    const char* protocol =
        co_http2_stream_get_protocol_mode(stream);
    if (protocol != NULL &&
        strcmp(protocol, "websocket") == 0)
    {
        co_ws_frame_t* ws_frame =
            co_http2_stream_receive_ws_frame(stream, request_data);

        if (ws_frame != NULL)
        {
            http_server_on_ws_http2_receive_frame(
                self, http2_client, stream, ws_frame);

            co_ws_frame_destroy(ws_frame);
        }

        return;
    }

    // method
    const char* method =
        co_http2_header_get_method(request_header);

    const co_http_url_st* url =
        co_http2_header_get_path_url(request_header);

    if (strcmp(url->path, "/") == 0 ||
        strcmp(url->path, "/index.html") == 0)
    {
        // query parameter
        co_string_map_t* query_map =
            co_http_url_query_parse(url->query, true);

        co_http2_header_t* response_header =
            co_http2_header_create_response(200);

        co_http2_header_add_field(
            response_header, "content-type", "text/html");
        co_http2_header_add_field(
            response_header, "cache-control", "no-store");

        const co_string_map_data_st* query_data1 =
            co_string_map_get(query_map, "param1");
        const co_string_map_data_st* query_data2 =
            co_string_map_get(query_map, "param2");

        char data[1024];
        sprintf(data,
            "<html>"
            "<head>"
            "<title>Coldforce Test (http/2)</title>"
            "<meta charset='utf-8'>"
            "</head>"
            "<body> Hello !!!<br>"
            ":method: %s<br>"
            "param1=%s<br>"
            "param2=%s<br>"
            "</body>"
            "</html>",
            method,
            (query_data1 ? query_data1->value : ""),
            (query_data2 ? query_data2->value : ""));
        size_t data_length = strlen(data);

        co_http2_stream_send_header(
            stream, false, response_header);
        co_http2_stream_send_data(
            stream, true, data, (uint32_t)data_length);

        co_string_map_destroy(query_map);
    }
    else if (strcmp(url->path, "/stop") == 0)
    {
        bool is_digest_auth_ok = false;

        const char* realm = "Coldforce Test Server";
        const char* user = "admin";
        const char* password = "12345";

        const char* nonce = "random_nonce";
        uint32_t nc = 0; // if nc == 0 then ignore nc

        const char* request_auth_str =
            co_http2_header_get_field(
                request_header, CO_HTTP2_HEADER_AUTHORIZATION);

        if (request_auth_str != NULL)
        {
            co_http_auth_t* request_auth =
                co_http_auth_create_request(request_auth_str);

            if (request_auth != NULL)
            {
                if (co_http_digest_auth_validate(
                    request_auth, "GET", "/stop", realm,
                    user, password,
                    nonce, nc))
                {
                    is_digest_auth_ok = true;
                }

                co_http_auth_destroy(request_auth);
            }
        }

        co_http2_header_t* response_header = NULL;
        const char* data = NULL;

        if (is_digest_auth_ok)
        {
            response_header =
                co_http2_header_create_response(200);

            data =
                "<html>"
                "<title>Coldforce Test (http/2)</title>"
                "<body> STOP OK </body>"
                "</html>";

            co_app_stop();
        }
        else
        {
            response_header =
                co_http2_header_create_response(401);

            data =
                "<html>"
                "<title>Coldforce Test (http/2)</title>"
                "<body> Unauthorized </body>"
                "</html>";

            co_http_auth_t* response_auth =
                co_http_digest_auth_create_response(realm, nonce, NULL);

            co_http2_header_apply_auth(response_header,
                CO_HTTP2_HEADER_WWW_AUTHENTICATE, response_auth);

            co_http_auth_destroy(response_auth);
        }

        co_http2_stream_send_header(
            stream, false, response_header);
        co_http2_stream_send_data(
            stream, true, data, (uint32_t)strlen(data));
    }
    else if (strcmp(url->path, "/server-push") == 0)
    {
    }
    else if (strcmp(url->path, "/ws-over-http2") == 0)
    {
        if (co_http2_header_validate_ws_connect_request(stream, request_header))
        {
            co_http2_header_t* response_header =
                co_http2_header_create_ws_connect_response(NULL, NULL);

            co_http2_stream_send_header(stream, true, response_header);

            co_http2_stream_set_protocol_mode(stream, "websocket");
        }
        else
        {
            co_http2_header_t* response_header =
                co_http2_header_create_response(400);

            const char* data =
                "<html>"
                "<title>Coldforce Test (http/2)</title>"
                "<body> Bad Request </body>"
                "</html>";
            size_t data_length = strlen(data);

            co_http2_stream_send_header(
                stream, false, response_header);
            co_http2_stream_send_data(
                stream, true, data, (uint32_t)data_length);
        }
    }
    else
    {
        co_http2_header_t* response_header =
            co_http2_header_create_response(404);

        const char* data =
            "<html>"
            "<title>Coldforce Test (http/2)</title>"
            "<body> Not Found </body>"
            "</html>";
        size_t data_length = strlen(data);

        co_http2_stream_send_header(
            stream, false, response_header);
        co_http2_stream_send_data(
            stream, true, data, (uint32_t)data_length);
    }
}

static void
http_server_on_http2_close(
    http_server_thread* self,
    co_http2_client_t* http2_client,
    int error_code
)
{
    (void)error_code;

    co_list_remove(self->http2_clients, http2_client);
}

void
add_http2_server_connection(
    http_server_thread* self,
    co_tcp_client_t* tcp_client
)
{
    co_http2_client_t* http2_client =
        co_tcp_upgrade_to_http2(tcp_client);

    co_http2_callbacks_st* callbacks =
        co_http2_get_callbacks(http2_client);
    callbacks->on_receive_finish =
        (co_http2_receive_finish_fn)http_server_on_http2_request;
    callbacks->on_close =
        (co_http2_close_fn)http_server_on_http2_close;

    co_http2_setting_param_st params[3];
    params[0].id = CO_HTTP2_SETTING_ID_INITIAL_WINDOW_SIZE;
    params[0].value = 1024 * 1024 * 10;
    params[1].id = CO_HTTP2_SETTING_ID_MAX_CONCURRENT_STREAMS;
    params[1].value = 200;
    params[2].id = CO_HTTP2_SETTING_ID_ENABLE_CONNECT_PROTOCOL;
    params[2].value = 1;
    co_http2_init_settings(http2_client, params, 3);

    co_list_add_tail(self->http2_clients, http2_client);
}
