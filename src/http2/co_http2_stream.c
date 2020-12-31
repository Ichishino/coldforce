#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_hpack.h>
#include <coldforce/http2/co_http2_message.h>

//---------------------------------------------------------------------------//
// http2 stream
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http2_stream_t*
co_http2_stream_create(
    uint32_t id,
    co_http2_client_t* client,
    co_http2_message_fn message_handler
)
{
    co_http2_stream_t* stream =
        (co_http2_stream_t*)co_mem_alloc(sizeof(co_http2_stream_t));

    if (stream == NULL)
    {
        return NULL;
    }

    stream->id = id;
    stream->state = CO_HTTP2_STREAM_STATE_IDLE;
    stream->client = client;

    stream->send_message = NULL;
    stream->receive_message = NULL;

    stream->on_message = message_handler;

    stream->header_block_pool.type = 0;
    stream->header_block_pool.data = NULL;

    stream->send_data_pool = NULL;
    stream->receive_data_pool = NULL;

    stream->local_window_size =
        client->local_settings.initial_window_size;
    stream->remote_window_size =
        client->remote_settings.initial_window_size;
    stream->current_window_size = stream->local_window_size;

    stream->promised_stream_id = 0;
    stream->data_save_fp = NULL;

    return stream;
}

void
co_http2_stream_destroy(
    co_http2_stream_t* stream
)
{
    if (stream != NULL)
    {
        co_http2_message_destroy(stream->send_message);
        co_http2_message_destroy(stream->receive_message);

        co_byte_array_destroy(stream->header_block_pool.data);

        co_byte_array_destroy(stream->send_data_pool);
        co_byte_array_destroy(stream->receive_data_pool);

        if (stream->data_save_fp != NULL)
        {
            fclose(stream->data_save_fp);
        }

        co_mem_free(stream);
    }
}

void
co_http2_stream_update_local_window_size(
    co_http2_stream_t* stream,
    uint32_t consumed_size
)
{
    if (stream->current_window_size > consumed_size)
    {
        stream->current_window_size -= consumed_size;
    }
    else
    {
        if ((stream->local_window_size * 2) < INT32_MAX)
        {
            stream->local_window_size *= 2;
        }
        else
        {
            stream->local_window_size = INT32_MAX;
        }

        stream->current_window_size = stream->local_window_size;

        co_http2_frame_t* frame =
            co_http2_create_window_update_frame(
                stream->local_window_size);

        co_http2_stream_send_frame(stream, frame);
    }
}

int
co_http2_stream_change_state(
    co_http2_stream_t* stream,
    bool send,
    const co_http2_frame_t* frame
)
{
    int error_code = CO_HTTP2_STREAM_ERROR_NO_ERROR;

    uint8_t frame_type = frame->header.type;

    bool end_stream =
        (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_STREAM);

    switch (stream->state)
    {
    case CO_HTTP2_STREAM_STATE_IDLE:
    {
        if (send)
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_HEADERS)
            {
                stream->state = CO_HTTP2_STREAM_STATE_OPEN;
            }
        }
        else
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_HEADERS)
            {
                stream->state = CO_HTTP2_STREAM_STATE_OPEN;
            }
            else if (frame_type != CO_HTTP2_FRAME_TYPE_PRIORITY)
            {
                error_code = CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR;
            }
        }

        break;
    }
    case CO_HTTP2_STREAM_STATE_OPEN:
    {
        if (send)
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else if (end_stream)
            {
                stream->state = CO_HTTP2_STREAM_STATE_LOCAL_CLOSED;
            }
        }
        else
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else if (end_stream)
            {
                stream->state = CO_HTTP2_STREAM_STATE_REMOTE_CLOSED;
            }
        }

        break;
    }
    case CO_HTTP2_STREAM_STATE_RESERVED_LOCAL:
    {
        if (send)
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_HEADERS)
            {
                stream->state = CO_HTTP2_STREAM_STATE_REMOTE_CLOSED;
            }
            else if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else
            {
                error_code = CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR;
            }
        }
        else
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else if (
                (frame_type != CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE) &&
                (frame_type != CO_HTTP2_FRAME_TYPE_PRIORITY))
            {
                error_code = CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR;
            }
        }

        break;
    }
    case CO_HTTP2_STREAM_STATE_RESERVED_REMOTE:
    {
        if (!send)
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_HEADERS)
            {
                stream->state = CO_HTTP2_STREAM_STATE_LOCAL_CLOSED;
            }
            else if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else
            {
                error_code = CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR;
            }
        }
        else
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else if (
                (frame_type != CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE) &&
                (frame_type != CO_HTTP2_FRAME_TYPE_PRIORITY))
            {
                error_code = CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR;
            }
        }

        break;
    }
    case CO_HTTP2_STREAM_STATE_REMOTE_CLOSED:
    {
        if (!send)
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else if (
                (frame_type != CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE) &&
                (frame_type != CO_HTTP2_FRAME_TYPE_PRIORITY))
            {
                error_code = CO_HTTP2_STREAM_ERROR_STREAM_CLOSED;
            }
        }
        else
        {
            if (end_stream ||
                (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM))
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
        }

        break;
    }
    case CO_HTTP2_STREAM_STATE_LOCAL_CLOSED:
    {
        if (send)
        {
            if (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM)
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
            else if (
                (frame_type != CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE) &&
                (frame_type != CO_HTTP2_FRAME_TYPE_PRIORITY))
            {
                error_code = CO_HTTP2_STREAM_ERROR_STREAM_CLOSED;
            }
        }
        else
        {
            if (end_stream ||
                (frame_type == CO_HTTP2_FRAME_TYPE_RST_STREAM))
            {
                stream->state = CO_HTTP2_STREAM_STATE_CLOSED;
            }
        }

        break;
    }
    case CO_HTTP2_STREAM_STATE_CLOSED:
    {
        if (!send)
        {
            if (frame_type != CO_HTTP2_FRAME_TYPE_PRIORITY)
            {
                error_code = CO_HTTP2_STREAM_STATE_CLOSED;
            }
        }
        else
        {
            error_code = CO_HTTP2_STREAM_STATE_CLOSED;
        }

        break;
    }
    default:
    {
        error_code = CO_HTTP2_STREAM_ERROR_INTERNAL_ERROR;

        break;
    }
    }

#ifdef CO_HTTP2_DEBUG
    if (error_code != CO_HTTP2_STREAM_ERROR_NO_ERROR)
    {
        char remote[64];
        co_net_addr_get_as_string(
            co_tcp_get_remote_net_addr(stream->client->tcp_client), remote);
        printf("[CO_HTTP2] <ERR> %s %s stream-%u *** state error: %d state(%d) frame(%d)\n",
            remote, (send ? "S->" : "->R"), stream->id, error_code, stream->state, frame->header.type);
    }
#endif

    return error_code;
}

void
co_http2_stream_frame_trace(
    co_http2_stream_t* stream,
    bool send,
    const co_http2_frame_t* frame
)
{
    char remote[64];
    co_net_addr_get_as_string(
        co_tcp_get_remote_net_addr(stream->client->tcp_client), remote);

    char info[256] = { 0 };
    char type[32] = { 0 };

    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_DATA:
    {
        sprintf(type, "DATA(%u)", frame->header.type);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_HEADERS:
    {
        sprintf(type, "HEAD(%u)", frame->header.type);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PRIORITY:
    {
        sprintf(type, "PRIO(%u)", frame->header.type);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_RST_STREAM:
    {
        sprintf(type, "RSTS(%u)", frame->header.type);

        sprintf(info, "error:%u",
            frame->payload.rst_stream.error_code);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_SETTINGS:
    {
        sprintf(type, "SETT(%u)", frame->header.type);

        for (size_t index = 0; index < frame->payload.settings.param_count; ++index)
        {
            char setting[32];
            sprintf(setting, "%u:%u ",
                frame->payload.settings.params[index].identifier,
                frame->payload.settings.params[index].value);

            strcat(info, setting);
        }
        
        break;
    }
    case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
    {
        sprintf(type, "PUSH(%u)", frame->header.type);

        sprintf(info, "promised_id:%u",
            frame->payload.push_promise.promised_stream_id);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PING:
    {
        sprintf(type, "PING(%u)", frame->header.type);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_GOAWAY:
    {
        sprintf(type, "GOAW(%u)", frame->header.type);

        sprintf(info, "last_id:%u error:%u",
            frame->payload.goaway.last_stream_id,
            frame->payload.goaway.error_code);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
    {
        sprintf(type, "WINU(%u)", frame->header.type);

        sprintf(info, "size_inc:%u",
            frame->payload.window_update.window_size_increment);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_CONTINUATION:
    {
        sprintf(type, "CONT(%u)", frame->header.type);

        break;
    }
    }

    printf("[CO_HTTP2] <INF> %s %s stream-%02u %s flags:0x%02x length:%u %s\n",
        remote, (send ? "S->" : "->R"),
        frame->header.stream_id, type,
        frame->header.flags, frame->header.length, info);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_http2_stream_send_frame(
    co_http2_stream_t* stream,
    co_http2_frame_t* frame
)
{
    if (co_http2_stream_change_state(
        stream, true, frame) != CO_HTTP2_STREAM_ERROR_NO_ERROR)
    {
        co_http2_frame_destroy(frame);

        return false;
    }

    frame->header.stream_id = stream->id;

#ifdef CO_HTTP2_DEBUG
    co_http2_stream_frame_trace(stream, true, frame);
#endif
    co_byte_array_t* buffer = co_byte_array_create();

    co_http2_frame_serialize(frame, buffer);

    bool result =
        co_http2_send_raw_data(stream->client,
            co_byte_array_get_ptr(buffer, 0),
            co_byte_array_get_count(buffer));

    co_byte_array_destroy(buffer);

    co_http2_frame_destroy(frame);

    return result;
}

bool
co_http2_stream_send_headers_frame(
    co_http2_stream_t* stream,
    bool end_stream,
    const co_http2_header_t* header,
    uint32_t stream_dependency,
    uint8_t weight
)
{
    if (stream->id == 0)
    {
        return false;
    }

    co_byte_array_t* request_data = co_byte_array_create();

    co_http2_hpack_serialize_header(
        header, &stream->client->remote_dynamic_table, request_data);

    const uint8_t* data_ptr =
        co_byte_array_get_const_ptr(request_data, 0);
    const uint32_t total_data_size =
        (const uint32_t)co_byte_array_get_count(request_data);

    const uint32_t max_frame_size =
        stream->client->remote_settings.max_frame_size;

    if (total_data_size > max_frame_size)
    {
        uint32_t index = 0;
        uint32_t data_size = max_frame_size;

        co_http2_frame_t* headers_frame =
            co_http2_create_headers_frame(
                end_stream, false, data_ptr, data_size,
                stream_dependency, weight,
                NULL, 0);

        if (!co_http2_stream_send_frame(stream, headers_frame))
        {
            co_byte_array_destroy(request_data);

            return false;
        }

        index += data_size;

        do
        {
            bool end_headers = false;

            if ((total_data_size - index) > max_frame_size)
            {
                data_size = max_frame_size;
            }
            else
            {
                data_size = total_data_size - index;

                end_headers = true;
            }

            co_http2_frame_t* continuation_frame =
                co_http2_create_continuation_frame(
                    end_headers, &data_ptr[index], data_size);

            if (!co_http2_stream_send_frame(stream, continuation_frame))
            {
                co_byte_array_destroy(request_data);

                return false;
            }

            index += data_size;

        } while (total_data_size > index);
    }
    else
    {
        co_http2_frame_t* headers_frame =
            co_http2_create_headers_frame(
                end_stream, true, data_ptr, total_data_size,
                stream_dependency, weight,
                NULL, 0);

        if (!co_http2_stream_send_frame(stream, headers_frame))
        {
            co_byte_array_destroy(request_data);

            return false;
        }
    }

    co_byte_array_destroy(request_data);

    return true;
}

bool
co_http2_stream_send_data_frame(
    co_http2_stream_t* stream,
    const co_http2_data_t* data
)
{
    if (stream->id == 0)
    {
        return false;
    }

    const uint8_t* data_ptr = (const uint8_t*)data->ptr;

    uint32_t total_data_size = (uint32_t)data->size;
    uint32_t max_frame_size =
        stream->client->remote_settings.max_frame_size;
    uint32_t window_size =
        co_min(stream->client->system_stream->remote_window_size,
            stream->remote_window_size);
    
    uint32_t pending_size = 0;

    if (total_data_size > window_size)
    {
        pending_size = total_data_size - window_size;
        total_data_size -= pending_size;
    }

    if (total_data_size > max_frame_size)
    {
        uint32_t index = 0;
        uint32_t data_size = max_frame_size;

        co_http2_frame_t* data_frame =
            co_http2_create_data_frame(
                false, data_ptr, data_size, NULL, 0);

        if (!co_http2_stream_send_frame(stream, data_frame))
        {
            return false;
        }

        index += data_size;

        do
        {
            bool end_stream = false;

            if ((total_data_size - index) > max_frame_size)
            {
                data_size = max_frame_size;
            }
            else
            {
                data_size = total_data_size - index;

                end_stream = true;
            }

            data_frame = co_http2_create_data_frame(
                end_stream, &data_ptr[index], data_size,
                NULL, 0);

            if (!co_http2_stream_send_frame(stream, data_frame))
            {
                return false;
            }

            index += data_size;

        } while (total_data_size > index);
    }
    else if (total_data_size > 0)
    {
        co_http2_frame_t* data_frame =
            co_http2_create_data_frame(
                true, data_ptr, total_data_size,
                NULL, 0);

        if (!co_http2_stream_send_frame(stream, data_frame))
        {
            return false;
        }
    }

    if (pending_size > 0)
    {
        co_byte_array_add(
            stream->send_data_pool,
            &data->ptr[window_size], pending_size);
    }

    return true;
}

void
co_http2_stream_on_receive_message(
    co_http2_stream_t* stream,
    int error_code
)
{
#ifdef CO_HTTP2_DEBUG
    if (error_code == 0)
    {
        printf("[CO_HTTP2] <INF> ========== RECEIVE ==========\n");
        co_http2_header_print(&stream->receive_message->header);
        printf("[CO_HTTP2] <INF> ======= CONTENT_SIZE(%zu)\n",
            stream->receive_message->data.size);
    }
#endif

    co_http2_message_fn handler = stream->on_message;

    if (handler != NULL)
    {
        handler(
            stream->client->tcp_client->sock.owner_thread,
            stream->client, stream, stream->receive_message,
            error_code);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_http2_stream_send_request_message(
    co_http2_stream_t* stream,
    co_http2_message_t* message
)
{
    if (stream->send_message != NULL)
    {
        return false;
    }

    stream->send_message = message;

    if (message->header.pseudo.authority == NULL)
    {
        message->header.pseudo.authority =
            co_http_url_create_host_and_port(stream->client->base_url);
    }

    if (message->header.pseudo.scheme == NULL)
    {
        message->header.pseudo.scheme =
            co_string_duplicate(stream->client->base_url->scheme);
    }

#ifdef CO_HTTP2_DEBUG
    printf("[CO_HTTP2] <INF> =========== SEND ============\n");
    co_http2_header_print(&message->header);
    printf("[CO_HTTP2] <INF> ======= CONTENT_SIZE(%zu)\n", message->data.size);
#endif

    uint32_t content_size =
        (uint32_t)co_http2_message_get_content_size(message);

    bool end_stream = (content_size == 0);

    bool result = co_http2_stream_send_headers_frame(
        stream, end_stream, &message->header, 0, 0);

    if (result && (content_size > 0))
    {
        result = co_http2_stream_send_data_frame(stream, &message->data);
    }

    return result;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_http2_stream_on_receive_frame(
    co_http2_stream_t* stream,
    co_http2_frame_t* frame
)
{
    int error_code =
        co_http2_stream_change_state(stream, false, frame);

    if (error_code != CO_HTTP2_STREAM_ERROR_NO_ERROR)
    {
        co_http2_frame_t* rst_frame =
            co_http2_create_rst_stream_frame(error_code);

        co_http2_stream_send_frame(stream, rst_frame);

        return false;
    }

    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_DATA:
    {
        if (stream->receive_message == NULL)
        {
            break;
        }

        co_http2_stream_update_local_window_size(stream, frame->header.length);

        if (stream->data_save_fp != NULL)
        {
            if (frame->payload.data.data_length > 0)
            {
                fwrite(frame->payload.data.data, 1,
                    frame->payload.data.data_length,
                    stream->data_save_fp);

                stream->receive_message->data.size +=
                    frame->payload.data.data_length;
            }

            if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_STREAM)
            {
                fclose(stream->data_save_fp);
                stream->data_save_fp = NULL;

                co_http2_stream_on_receive_message(stream, 0);
            }
        }
        else if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_STREAM)
        {
            if ((stream->receive_data_pool == NULL) ||
                (co_byte_array_get_count(stream->receive_data_pool) == 0))
            {
                stream->receive_message->data.ptr =
                    frame->payload.data.data;
                stream->receive_message->data.size =
                    frame->payload.data.data_length;

                frame->payload.data.data = NULL;
                frame->payload.data.data_length = 0;
            }
            else
            {
                if (frame->payload.data.data_length > 0)
                {
                    co_byte_array_add(
                        stream->receive_data_pool,
                        frame->payload.data.data,
                        frame->payload.data.data_length);
                }
                
                if (co_byte_array_get_count(stream->receive_data_pool) > 0)
                {
                    co_byte_array_add(stream->receive_data_pool, "\0", 1);

                    stream->receive_message->data.size =
                        (uint32_t)co_byte_array_get_count(stream->receive_data_pool) - 1;
                    stream->receive_message->data.ptr =
                        co_byte_array_detach(stream->receive_data_pool);
                }
                else
                {
                    stream->receive_message->data.ptr = NULL;
                    stream->receive_message->data.size = 0;
                }
            }

            co_http2_stream_on_receive_message(stream, 0);
        }
        else
        {
            if (stream->receive_data_pool == NULL)
            {
                stream->receive_data_pool = co_byte_array_create();
            }

            if (frame->payload.data.data_length > 0)
            {
                co_byte_array_add(
                    stream->receive_data_pool,
                    frame->payload.data.data,
                    frame->payload.data.data_length);
            }
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_HEADERS:
    {
        if (stream->receive_message != NULL)
        {
            break;
        }        

        stream->receive_message = co_http2_message_create();

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_HEADERS)
        {
            co_http2_hpack_deserialize_header(
                frame->payload.headers.header_block_fragment,
                frame->payload.headers.header_block_fragment_length,
                &stream->client->local_dynamic_table,
                &stream->receive_message->header);

            co_byte_array_clear(stream->header_block_pool.data);

            if (stream->state == CO_HTTP2_STREAM_STATE_REMOTE_CLOSED)
            {
                co_http2_stream_on_receive_message(stream, 0);
            }
            else
            {
                if (stream->send_message->data.file_path != NULL)
                {
                    stream->data_save_fp = fopen(
                        stream->send_message->data.file_path, "wb");

                    if (stream->data_save_fp == NULL)
                    {
                        co_http2_frame_t* rst_frame =
                            co_http2_create_rst_stream_frame(
                                CO_HTTP2_STREAM_ERROR_INTERNAL_ERROR);

                        co_http2_stream_send_frame(stream, rst_frame);

                        co_http2_stream_on_receive_message(
                            stream, CO_HTTP2_ERROR_FILE_IO);
                    }
                }
            }
        }
        else
        {
            if (stream->header_block_pool.data == NULL)
            {
                stream->header_block_pool.data = co_byte_array_create();
            }

            stream->header_block_pool.type = CO_HTTP2_FRAME_TYPE_HEADERS;

            if (frame->payload.headers.header_block_fragment_length > 0)
            {
                co_byte_array_add(
                    stream->header_block_pool.data,
                    frame->payload.headers.header_block_fragment,
                    frame->payload.headers.header_block_fragment_length);
            }
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PRIORITY:
    {
        break;
    }
    case CO_HTTP2_FRAME_TYPE_RST_STREAM:
    {
        co_http2_stream_on_receive_message(stream,
            (CO_HTTP2_ERROR_STREAM_CLOSED -
                frame->payload.rst_stream.error_code));
        
        break;
    }
    case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
    {
        stream->promised_stream_id = frame->payload.push_promise.promised_stream_id;

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_HEADERS)
        {
            co_http2_message_t* message = co_http2_message_create();

            co_http2_hpack_deserialize_header(
                frame->payload.push_promise.header_block_fragment,
                frame->payload.push_promise.header_block_fragment_length,
                &stream->client->local_dynamic_table,
                &message->header);

            co_http2_client_on_push_promise_message(
                stream->client, stream, stream->promised_stream_id, message);
        }
        else
        {
            if (stream->header_block_pool.data == NULL)
            {
                stream->header_block_pool.data = co_byte_array_create();
            }

            stream->header_block_pool.type = CO_HTTP2_FRAME_TYPE_PUSH_PROMISE;

            if (frame->payload.push_promise.header_block_fragment_length > 0)
            {
                co_byte_array_add(
                    stream->header_block_pool.data,
                    frame->payload.push_promise.header_block_fragment,
                    frame->payload.push_promise.header_block_fragment_length);
            }
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
    {
        stream->remote_window_size =
            frame->payload.window_update.window_size_increment;

        break;
    }
    case CO_HTTP2_FRAME_TYPE_CONTINUATION:
    {
        if (stream->receive_message == NULL)
        {
            break;
        }

        if (frame->payload.continuation.header_block_fragment_length > 0)
        {
            co_byte_array_add(
                stream->header_block_pool.data,
                frame->payload.continuation.header_block_fragment,
                frame->payload.continuation.header_block_fragment_length);
        }
        
        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_HEADERS)
        {
            const uint8_t* data =
                co_byte_array_get_const_ptr(stream->header_block_pool.data, 0);
            size_t data_size = co_byte_array_get_count(stream->header_block_pool.data);

            co_http2_message_t* message;
                
            if (stream->header_block_pool.type == CO_HTTP2_FRAME_TYPE_HEADERS)
            {
                message = stream->receive_message;
            }
            else
            {
                message = co_http2_message_create();
            }

            co_http2_hpack_deserialize_header(
                data, data_size,
                &stream->client->local_dynamic_table,
                &message->header);

            if (stream->header_block_pool.type == CO_HTTP2_FRAME_TYPE_PUSH_PROMISE)
            {
                co_http2_client_on_push_promise_message(
                    stream->client, stream, stream->promised_stream_id, message);

                stream->promised_stream_id = 0;
            }
            else if (stream->state == CO_HTTP2_STREAM_STATE_REMOTE_CLOSED)
            {
                co_http2_stream_on_receive_message(stream, 0);
            }

            stream->header_block_pool.type = 0;
            co_byte_array_clear(stream->header_block_pool.data);
        }

        break;
    }
    default:
    {
        co_http2_frame_t* rst_frame =
            co_http2_create_rst_stream_frame(
                CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR);

        co_http2_stream_send_frame(stream, rst_frame);

        return false;
    }
    }

    return true;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

const co_http2_message_t*
co_http2_stream_get_send_message(
    const co_http2_stream_t* stream
)
{
    return stream->send_message;
}
