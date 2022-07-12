#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_byte_order.h>

#include <coldforce/http/co_base64.h>

#include <coldforce/http2/co_http2_http_extension.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_server.h>

//---------------------------------------------------------------------------//
// http extension for http2
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http2_client_t*
co_http_upgrade_to_http2(
    co_http_client_t* http_client
)
{
    co_http2_client_t* http2_client =
        (co_http2_client_t*)co_mem_alloc(sizeof(co_http2_client_t));

    if (http2_client == NULL)
    {
        return NULL;
    }

    co_tcp_receive_fn receive_handler;
    co_http2_settings_st* settings = NULL;
    uint32_t new_stream_id = 0;

    if (http_client->base_url != NULL)
    {
        new_stream_id = UINT32_MAX;

        receive_handler =
            (co_tcp_receive_fn)co_http2_client_on_tcp_receive_ready;

        settings = &http2_client->local_settings;
    }
    else
    {
        receive_handler =
            (co_tcp_receive_fn)co_http2_server_on_tcp_receive_ready;

        settings = &http2_client->remote_settings;
    }

    http2_client->base_url = http_client->base_url;
    http_client->base_url = NULL;

    http2_client->tcp_client = http_client->tcp_client;
    http_client->tcp_client = NULL;

    co_http2_client_setup(http2_client);

    co_byte_array_t* receive_data = http2_client->receive_data;
    http2_client->receive_data = http_client->receive_data;
    http2_client->receive_data_index = http_client->receive_data_index;
    http_client->receive_data = receive_data;

    http2_client->new_stream_id = new_stream_id;

    const co_http_header_t* header =
        co_http_request_get_header(http_client->request);
    const char* http2_settings =
        co_http_header_get_field(
            header, CO_HTTP2_HEADER_SETTINGS);

    if (http2_settings != NULL)
    {
        co_http2_set_upgrade_settings(
            http2_settings, strlen(http2_settings), settings);
    }

    http2_client->tcp_client->callbacks.on_receive =
        receive_handler;
    http2_client->tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_http2_client_on_tcp_close;

    co_http_client_destroy(http_client);

    return http2_client;
}

bool
co_http_request_is_connection_preface(
    const co_http_request_t* request
)
{
    const char* method = co_http_request_get_method(request);

    if ((method == NULL) ||
        (strcmp(method, "PRI") != 0))
    {
        return false;
    }

    const char* path = co_http_request_get_path(request);

    if ((path == NULL) ||
        (strcmp(path, "*") != 0))
    {
        return false;
    }

    const char* version = co_http_request_get_version(request);

    if ((version == NULL) ||
        (strcmp(version, "HTTP/2.0") != 0))
    {
        return false;
    }

    return true;
}

bool
co_http_request_is_http2_upgrade(
    const co_http_request_t* request
)
{
    const co_http_header_t* header =
        co_http_request_get_const_header(request);

    const char* connection =
        co_http_header_get_field(
            header, CO_HTTP_HEADER_CONNECTION);
    const char* upgrade =
        co_http_header_get_field(
            header, CO_HTTP_HEADER_UPGRADE);
    const char* http2_settings =
        co_http_header_get_field(
            header, CO_HTTP2_HEADER_SETTINGS);

    if ((connection == NULL) ||
        (upgrade == NULL) ||
        (http2_settings == NULL))
    {
        return false;
    }

    if (strcmp(upgrade, CO_HTTP2_UPGRADE) != 0)
    {
        return false;
    }

    return true;
}

co_http_request_t*
co_http_request_create_http2_upgrade(
    const co_http2_client_t* client,
    const char* path,
    const co_http2_setting_param_st* param,
    uint16_t param_count
)
{
    char* b64_str = NULL;
    size_t b64_str_length = 0;

    if (param_count > 0)
    {
        co_byte_array_t* buffer = co_byte_array_create();

        for (uint16_t param_index = 0;
            param_index < param_count; ++param_index)
        {
            uint16_t u16 =
                co_byte_order_16_host_to_network(
                    param[param_index].id);
            co_byte_array_add(
                buffer, &u16, sizeof(uint16_t));

            uint32_t u32 =
                co_byte_order_32_host_to_network(
                    param[param_index].value);
            co_byte_array_add(
                buffer, &u32, sizeof(uint32_t));
        }

        co_base64url_encode(
            co_byte_array_get_const_ptr(buffer, 0),
            co_byte_array_get_count(buffer),
            &b64_str, &b64_str_length,
            false);

        co_byte_array_destroy(buffer);
    }

    co_http_request_t* request =
        co_http_request_create_with("GET", path);

    co_http_header_t* header =
        co_http_request_get_header(request);

    char* host_and_port =
        co_http_url_create_host_and_port(client->base_url);

    co_http_header_add_field_ptr(
        header, co_string_duplicate(CO_HTTP_HEADER_HOST), host_and_port);

    co_http_header_add_field(
        header, CO_HTTP_HEADER_CONNECTION,
        CO_HTTP_HEADER_UPGRADE", "CO_HTTP2_HEADER_SETTINGS);
    co_http_header_add_field(
        header, CO_HTTP_HEADER_UPGRADE,
        CO_HTTP2_UPGRADE);
    co_http_header_add_field_ptr(
        header, co_string_duplicate(CO_HTTP2_HEADER_SETTINGS),
        ((b64_str != NULL) ? b64_str : co_string_duplicate("")));

    return request;
}

co_http_response_t*
co_http_response_create_http2_upgrade(
    uint16_t status_code,
    const char* reason_phrase
)
{
    co_http_response_t* response =
        co_http_response_create_with(status_code, reason_phrase);

    if (status_code == 101)
    {
        co_http_header_t* header =
            co_http_response_get_header(response);

        co_http_header_add_field(header,
            CO_HTTP_HEADER_UPGRADE, CO_HTTP2_UPGRADE);
        co_http_header_add_field(header,
            CO_HTTP_HEADER_CONNECTION, CO_HTTP_HEADER_UPGRADE);
    }

    return response;
}
