#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>
#include <coldforce/core/co_random.h>
#include <coldforce/core/co_string_token.h>

#include <coldforce/http/co_base64.h>
#include <coldforce/http/co_sha1.h>
#include <coldforce/http/co_http_server.h>

#include <coldforce/ws/co_ws_server.h>
#include <coldforce/ws/co_ws_client.h>
#include <coldforce/ws/co_ws_http_extension.h>
#include <coldforce/ws/co_ws_log.h>

//---------------------------------------------------------------------------//
// http extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static char*
co_ws_create_base64_accept_key(
    const char* ws_key
)
{
    size_t key_data_size = strlen(ws_key) + 36;

    char* key_data = co_mem_alloc(key_data_size + 1);

    if (key_data == NULL)
    {
        return NULL;
    }

    strcpy(key_data, ws_key);
    strcat(key_data, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    uint8_t sha1_hash[CO_SHA1_HASH_SIZE];

    co_sha1(key_data, (uint32_t)key_data_size, sha1_hash);

    co_mem_free(key_data);

    char* base64_data;
    size_t base64_data_length;

    co_base64_encode(
        sha1_hash, sizeof(sha1_hash),
        &base64_data, &base64_data_length, true);

    return base64_data;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_ws_client_t*
co_http_upgrade_to_ws(
    co_http_client_t* http_client
)
{
    co_ws_client_t* ws_client =
        (co_ws_client_t*)co_mem_alloc(sizeof(co_ws_client_t));

    if (ws_client == NULL)
    {
        return NULL;
    }

    co_http_connection_move(
        &http_client->conn, &ws_client->conn);

    co_ws_client_setup(ws_client);

    bool is_server =
        co_http_connection_is_server(&ws_client->conn);

    ws_client->mask = !is_server;

    if (is_server)
    {
        ws_client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_ws_server_on_tcp_receive_ready;
    }
    else
    {
        ws_client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_ws_client_on_tcp_receive_ready;
    }

    ws_client->conn.tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_ws_client_on_tcp_close;

    co_http_client_destroy(http_client);

    return ws_client;
}

bool
co_http_request_validate_ws_upgrade(
    const co_http_request_t* request
)
{
    const co_http_header_t* header =
        co_http_request_get_const_header(request);

    const char* upgrade =
        co_http_header_get_field(header, CO_HTTP_HEADER_UPGRADE);

    if (upgrade == NULL)
    {
        return false;
    }

    if (co_string_case_compare(upgrade, "websocket") != 0)
    {
        return false;
    }

    const char* connection =
        co_http_header_get_field(header, CO_HTTP_HEADER_CONNECTION);

    if (connection == NULL)
    {
        return false;
    }

    co_string_token_st tokens[8];
    size_t token_count = co_string_token_split(connection, tokens, 8);

    bool upgrade_contains =
        co_string_token_contains(tokens, token_count, "upgrade");

    co_string_token_cleanup(tokens, token_count);

    if (!upgrade_contains)
    {
        return false;
    }

    const char* ws_key =
        co_http_header_get_field(header, CO_HTTP_HEADER_SEC_WS_KEY);

    if (ws_key == NULL)
    {
        return false;
    }

    uint8_t* nonce;
    size_t nonce_length;

    if (!co_base64_decode(
        ws_key, strlen(ws_key), &nonce, &nonce_length))
    {
        return false;
    }

    co_mem_free(nonce);

    if (nonce_length != 16)
    {
        return false;
    }

    const char* ws_version =
        co_http_header_get_field(header, CO_HTTP_HEADER_SEC_WS_VERSION);

    if (ws_version == NULL)
    {
        return false;
    }

    if (strcmp(ws_version, "13") != 0)
    {
        return false;
    }

    return true;
}

bool
co_http_response_validate_ws_upgrade(
    const co_http_response_t* response,
    const co_http_request_t* request
)
{
    const co_http_header_t* header =
        co_http_response_get_const_header(response);

    const char* upgrade =
        co_http_header_get_field(header, CO_HTTP_HEADER_UPGRADE);

    if (upgrade == NULL)
    {
        return false;
    }

    if (co_string_case_compare(upgrade, "websocket") != 0)
    {
        return false;
    }

    const char* connection =
        co_http_header_get_field(header, CO_HTTP_HEADER_CONNECTION);

    if (connection == NULL)
    {
        return false;
    }

    co_string_token_st tokens[8];
    size_t token_count = co_string_token_split(connection, tokens, 8);

    bool upgrade_contains =
        co_string_token_contains(tokens, token_count, "upgrade");

    co_string_token_cleanup(tokens, token_count);

    if (!upgrade_contains)
    {
        return false;
    }

    const char* response_key =
        co_http_header_get_field(header, CO_HTTP_HEADER_SEC_WS_ACCEPT);

    if (response_key == NULL)
    {
        return false;
    }

    const co_http_header_t* request_header =
        co_http_request_get_const_header(request);
    const char* key = co_http_header_get_field(
        request_header, CO_HTTP_HEADER_SEC_WS_KEY);

    char* request_key =
        co_ws_create_base64_accept_key(key);

    if (strcmp(response_key, request_key) != 0)
    {
        co_string_destroy(request_key);

        return false;
    }

    co_string_destroy(request_key);

    uint16_t status_code =
        co_http_response_get_status_code(response);

    return (status_code == 101) ? true : false;
}

co_http_request_t*
co_http_request_create_ws_upgrade(
    const char* path,
    const char* protocols,
    const char* extensions
)
{
    co_http_request_t* request =
        co_http_request_create("GET", path);

    co_http_header_t* header =
        co_http_request_get_header(request);

    co_http_header_add_field(
        header, CO_HTTP_HEADER_UPGRADE, "websocket");
    co_http_header_add_field(
        header, CO_HTTP_HEADER_CONNECTION, "keep-alive, upgrade");

    uint8_t ws_key[16];
    co_random(ws_key, sizeof(ws_key));

    char* base64_data;
    size_t base64_data_length;

    co_base64_encode(ws_key, sizeof(ws_key),
        &base64_data, &base64_data_length, true);

    co_http_header_add_field(
        header, CO_HTTP_HEADER_SEC_WS_KEY, base64_data);

    co_string_destroy(base64_data);

    co_http_header_add_field(
        header, CO_HTTP_HEADER_SEC_WS_VERSION, "13");

    if (protocols != NULL)
    {
        co_http_header_add_field(
            header, CO_HTTP_HEADER_SEC_WS_PROTOCOL, protocols);
    }

    if (extensions != NULL)
    {
        co_http_header_add_field(
            header, CO_HTTP_HEADER_SEC_WS_EXTENSIONS, extensions);
    }

    return request;
}

co_http_response_t*
co_http_response_create_ws_upgrade(
    const co_http_request_t* request,
    const char* protocol,
    const char* extension
)
{
    co_http_response_t* response =
        co_http_response_create(101, "Switching Protocols");

    co_http_header_t* header =
        co_http_response_get_header(response);

    co_http_header_add_field(
        header, CO_HTTP_HEADER_UPGRADE, "websocket");
    co_http_header_add_field(
        header, CO_HTTP_HEADER_CONNECTION, "upgrade");

    const co_http_header_t* request_header =
        co_http_request_get_const_header(request);
    const char* ws_key =
        co_http_header_get_field(request_header, CO_HTTP_HEADER_SEC_WS_KEY);

    if (ws_key == NULL)
    {
        co_http_response_destroy(response);

        return NULL;
    }

    char* accept_key = co_ws_create_base64_accept_key(ws_key);

    co_http_header_add_field(
        header, CO_HTTP_HEADER_SEC_WS_ACCEPT, accept_key);

    co_string_destroy(accept_key);

    if (protocol != NULL)
    {
        co_http_header_add_field(
            header, CO_HTTP_HEADER_SEC_WS_PROTOCOL, protocol);
    }

    if (extension != NULL)
    {
        co_http_header_add_field(
            header, CO_HTTP_HEADER_SEC_WS_EXTENSIONS, extension);
    }

    return response;
}
