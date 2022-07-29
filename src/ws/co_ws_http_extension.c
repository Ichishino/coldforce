#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_base64.h>
#include <coldforce/http/co_sha1.h>
#include <coldforce/http/co_random.h>
#include <coldforce/http/co_http_string_list.h>
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

    co_ws_client_setup(ws_client,
        http_client->conn.tcp_client, http_client->conn.base_url);

    ws_client->mask = (ws_client->conn.base_url != NULL);

    http_client->conn.tcp_client = NULL;
    http_client->conn.base_url = NULL;

    co_http_client_destroy(http_client);

    if (ws_client->conn.base_url != NULL)
    {
        ws_client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_ws_client_on_receive_ready;
    }
    else
    {
        ws_client->conn.tcp_client->callbacks.on_receive =
            (co_tcp_receive_fn)co_ws_server_on_receive_ready;
    }

    ws_client->conn.tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_ws_client_on_close;

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

    co_http_string_item_st items[8];
    size_t item_count = co_http_string_list_parse(connection, items, 8);

    bool upgrade_contains =
        co_http_string_list_contains(items, item_count, "upgrade");

    co_http_string_list_cleanup(items, item_count);

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

    co_http_string_item_st items[8];
    size_t item_count = co_http_string_list_parse(connection, items, 8);

    bool upgrade_contains =
        co_http_string_list_contains(items, item_count, "upgrade");

    co_http_string_list_cleanup(items, item_count);

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
        co_http_request_create_with("GET", path);

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
    const char* extensions
)
{
    co_http_response_t* response =
        co_http_response_create_with(101, "Switching Protocols");

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

    if (extensions != NULL)
    {
        co_http_header_add_field(
            header, CO_HTTP_HEADER_SEC_WS_EXTENSIONS, extensions);
    }

    return response;
}

bool
co_http_connection_send_ws_frame(
    co_http_connection_t* conn,
    bool fin,
    uint8_t opcode,
    bool mask,
    const void* data,
    size_t data_size
)
{
    co_ws_log_debug_frame(
        &conn->tcp_client->sock.local_net_addr,
        "-->",
        &conn->tcp_client->remote_net_addr,
        fin, opcode, data_size,
        "ws send frame");

    uint8_t header[32];
    uint32_t header_size = CO_WS_FRAME_HEADER_MIN_SIZE;

    header[0] = opcode;

    if (fin)
    {
        header[0] |= 0x80;
    }

    if (data_size <= 125)
    {
        header[1] = (uint8_t)data_size;
    }
    else if (data_size <= UINT16_MAX)
    {
        header[1] = 126;

        header[2] = (uint8_t)((data_size & 0xff00) >> 8);
        header[3] = (uint8_t)(data_size & 0x00ff);

        header_size += 2;
    }
    else
    {
        header[1] = 127;

        header[2] = (uint8_t)((data_size & 0xff00000000000000) >> 56);
        header[3] = (uint8_t)((data_size & 0x00ff000000000000) >> 48);
        header[4] = (uint8_t)((data_size & 0x0000ff0000000000) >> 40);
        header[5] = (uint8_t)((data_size & 0x000000ff00000000) >> 32);
        header[6] = (uint8_t)((data_size & 0x00000000ff000000) >> 24);
        header[7] = (uint8_t)((data_size & 0x0000000000ff0000) >> 16);
        header[8] = (uint8_t)((data_size & 0x000000000000ff00) >> 8);
        header[9] = (uint8_t)(data_size & 0x00000000000000ff);

        header_size += 8;
    }

    bool result = false;

    if (mask)
    {
        header[1] |= 0x80;

        uint8_t* masked_data =
            (uint8_t*)co_mem_alloc(data_size);

        if (masked_data == NULL)
        {
            return false;
        }

        uint8_t mask_key[CO_WS_FRAME_MASK_SIZE];

        co_random(mask_key, sizeof(mask_key));

        for (uint16_t index = 0;
            index < CO_WS_FRAME_MASK_SIZE; ++index)
        {
            header[header_size] = mask_key[index];
            ++header_size;
        }

        for (uint64_t index = 0; index < data_size; ++index)
        {
            masked_data[index] = ((uint8_t*)data)[index] ^
                mask_key[index % CO_WS_FRAME_MASK_SIZE];
        }

        result =
            co_http_connection_send_data(
                conn, header, header_size);

        if (result && (data_size > 0))
        {
            result =
                co_http_connection_send_data(
                    conn, masked_data, data_size);
        }

        co_mem_free(masked_data);
    }
    else
    {
        result =
            co_http_connection_send_data(
                conn, header, header_size);

        if (result && (data_size > 0))
        {
            result =
                co_http_connection_send_data(
                    conn, data, data_size);
        }
    }

    return result;
}
