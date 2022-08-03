#include "test_http_server_ws_connection.h"

static void
http_server_on_ws_receive_frame(
    http_server_thread* self,
    co_ws_client_t* ws_client,
    const co_ws_frame_t* frame,
    int error_code
)
{
    if (error_code != 0)
    {
        co_list_remove(self->ws_clients, ws_client);

        return;
    }

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
        co_ws_send(ws_client,
            fin, opcode, data, (size_t)data_size);

        break;
    }
    default:
    {
        co_ws_default_handler(ws_client, frame);

        break;
    }
    }
}

static void
http_server_on_ws_close(
    http_server_thread* self,
    co_ws_client_t* ws_client
)
{
    co_list_remove(self->ws_clients, ws_client);
}

void
add_ws_server_connection(
    http_server_thread* self,
    co_http_client_t* http_client
)
{
    co_ws_client_t* ws_client =
        co_http_upgrade_to_ws(http_client);

    co_ws_callbacks_st* callbacks =
        co_ws_get_callbacks(ws_client);
    callbacks->on_receive_frame =
        (co_ws_receive_frame_fn)http_server_on_ws_receive_frame;
    callbacks->on_close =
        (co_ws_close_fn)http_server_on_ws_close;

    co_list_add_tail(self->ws_clients, ws_client);
}
