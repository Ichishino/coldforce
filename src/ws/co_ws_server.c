#include <coldforce/core/co_std.h>

#include <coldforce/http/co_http_log.h>

#include <coldforce/ws/co_ws_client.h>
#include <coldforce/ws/co_ws_server.h>
#include <coldforce/ws/co_ws_http_extension.h>
#include <coldforce/ws/co_ws_log.h>

//---------------------------------------------------------------------------//
// websocket server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static bool
co_ws_server_on_receive_http_request(
    co_thread_t* thread,
    co_ws_client_t* client
)
{
    co_http_request_t* request = co_http_request_create();

    int parse_result =
        co_http_request_deserialize(request,
            client->conn.receive_data.ptr,
            &client->conn.receive_data.index);

    if (parse_result == CO_HTTP_PARSE_MORE_DATA)
    {
        co_http_request_destroy(request);

        return true;
    }
    else if (parse_result != CO_HTTP_PARSE_COMPLETE)
    {
        co_http_request_destroy(request);

        return false;
    }

    co_http_log_debug_request_header(
        &client->conn.tcp_client->sock.local_net_addr,
        "<--",
        &client->conn.tcp_client->remote_net_addr,
        request,
        "http receive request");

    bool result = co_http_request_validate_ws_upgrade(request);

    if (client->callbacks.on_upgrade != NULL)
    {
        co_ws_upgrade_fn handler = client->callbacks.on_upgrade;
        client->callbacks.on_upgrade = NULL;

        handler(thread, client,
            (const co_http_message_t*)request,
            result ? 0 : CO_WS_ERROR_INVALID_UPGRADE);
    }
    else
    {
        if (result)
        {
            co_http_response_t* response =
                co_http_response_create_ws_upgrade(
                    request, NULL, NULL);

            co_http_connection_send_response(
                (co_http_connection_t*)client, response);

            co_http_response_destroy(response);
        }
    }

    co_http_request_destroy(request);

    return result;
}

void
co_ws_server_on_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)tcp_client->sock.sub_class;

    ssize_t receive_result =
        client->conn.module.receive_all(
            client->conn.tcp_client,
            client->conn.receive_data.ptr);

    if (receive_result <= 0)
    {
        return;
    }

    size_t data_size =
        co_byte_array_get_count(client->conn.receive_data.ptr);

    while (data_size > client->conn.receive_data.index)
    {
        if ((data_size - client->conn.receive_data.index) <
            CO_WS_FRAME_HEADER_MIN_SIZE)
        {
            return;
        }

        co_ws_frame_t* frame = co_ws_frame_create();

        int result = co_ws_frame_deserialize(frame,
            co_byte_array_get_ptr(client->conn.receive_data.ptr, 0),
            co_byte_array_get_count(client->conn.receive_data.ptr),
            &client->conn.receive_data.index);

        if (result == CO_WS_PARSE_COMPLETE)
        {
            co_ws_log_debug_frame(
                &client->conn.tcp_client->sock.local_net_addr,
                "<--",
                &client->conn.tcp_client->remote_net_addr,
                frame->header.fin,
                frame->header.opcode,
                frame->payload_data,
                (size_t)frame->header.payload_size,
                "ws receive frame");

            co_ws_client_on_frame(
                thread, client, frame, 0);

            if (client->conn.tcp_client == NULL)
            {
                return;
            }

            continue;
        }
        else if (result == CO_WS_PARSE_MORE_DATA)
        {
            co_ws_frame_destroy(frame);

            return;
        }
        else
        {
            co_ws_frame_destroy(frame);

            if (co_ws_server_on_receive_http_request(thread, client))
            {
                if (client->conn.tcp_client != NULL)
                {
                    continue;
                }
            }
            else
            {
                co_ws_client_on_frame(
                    thread, client, NULL, result);
            }

            return;
        }
    }

    client->conn.receive_data.index = 0;
    co_byte_array_clear(client->conn.receive_data.ptr);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_ws_client_t*
co_tcp_upgrade_to_ws(
    co_tcp_client_t* tcp_client
)
{
    co_ws_client_t* ws_client =
        (co_ws_client_t*)co_mem_alloc(sizeof(co_ws_client_t));

    if (ws_client == NULL)
    {
        return NULL;
    }

    ws_client->conn.tcp_client = tcp_client;

    co_ws_client_setup(ws_client, tcp_client, NULL);

    ws_client->mask = false;

    ws_client->conn.tcp_client->callbacks.on_receive =
        (co_tcp_receive_fn)co_ws_server_on_receive_ready;
    ws_client->conn.tcp_client->callbacks.on_close =
        (co_tcp_close_fn)co_ws_client_on_close;

    return ws_client;
}
