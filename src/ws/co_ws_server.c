#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_ws_client.h>
#include <coldforce/ws/co_ws_server.h>
#include <coldforce/ws/co_ws_http_extension.h>

//---------------------------------------------------------------------------//
// websocket server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_ws_server_on_receive_ready(
    co_thread_t* thread,
    co_tcp_client_t* tcp_client
)
{
    co_ws_client_t* client =
        (co_ws_client_t*)tcp_client->sock.sub_class;

    ssize_t receive_result = client->module.receive_all(
        client->tcp_client, client->receive_data);

    if (receive_result <= 0)
    {
        return;
    }

    size_t data_size =
        co_byte_array_get_count(client->receive_data);

    while (data_size > client->receive_data_index)
    {
        if ((data_size - client->receive_data_index) <
            CO_WS_FRAME_HEADER_MIN_SIZE)
        {
            return;
        }

        co_ws_frame_t* frame = co_ws_frame_create();

        int result = co_ws_frame_deserialize(frame,
            client->receive_data, &client->receive_data_index);

        if (result == CO_WS_PARSE_COMPLETE)
        {
            co_ws_client_on_frame(
                thread, client, frame, 0);

            if (client->tcp_client == NULL)
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

            co_http_request_t* request = co_http_request_create();

            if (co_http_request_deserialize(request,
                client->receive_data, &client->receive_data_index) ==
                CO_HTTP_PARSE_COMPLETE)
            {
                if (co_http_request_validate_ws_upgrade(request))
                {
                    co_http_response_t* response =
                        co_http_response_create_ws_upgrade(
                            request, NULL, NULL);
                    co_http_response_set_version(
                        response, CO_HTTP_VERSION_1_1);

                    co_byte_array_t* buffer = co_byte_array_create();
                    co_http_response_serialize(response, buffer);

                    co_ws_send_raw_data(client,
                        co_byte_array_get_ptr(buffer, 0),
                        co_byte_array_get_count(buffer));

                    co_byte_array_destroy(buffer);
                    co_http_response_destroy(response);
                    co_http_request_destroy(request);

                    continue;
                }
            }

            co_http_request_destroy(request);

            co_ws_client_on_frame(
                thread, client, NULL, result);

            return;
        }
    }

    client->receive_data_index = 0;
    co_byte_array_clear(client->receive_data);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_ws_client_t*
co_ws_client_create_with(
    co_tcp_client_t* tcp_client
)
{
    co_ws_client_t* ws_client =
        (co_ws_client_t*)co_mem_alloc(sizeof(co_ws_client_t));

    if (ws_client == NULL)
    {
        return NULL;
    }

    ws_client->tcp_client = tcp_client;

    co_ws_client_setup(ws_client, tcp_client, NULL);

    ws_client->mask = false;

    co_tcp_set_receive_handler(
        ws_client->tcp_client,
        (co_tcp_receive_fn)co_ws_server_on_receive_ready);
    co_tcp_set_close_handler(
        ws_client->tcp_client,
        (co_tcp_close_fn)co_ws_client_on_close);

    return ws_client;
}
