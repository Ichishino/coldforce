#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_client.h>
#include <coldforce/http2/co_http2_hpack.h>

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

    stream->send_header = NULL;
    stream->receive_header = NULL;

    stream->receive_data.ptr = NULL;
    stream->receive_data.size = 0;
    stream->receive_data.file_path = NULL;

    stream->on_message = message_handler;

    stream->header_block_pool.type = 0;
    stream->header_block_pool.data = NULL;

    stream->receive_data_pool = NULL;

    stream->max_local_window_size =
        client->local_settings.initial_window_size;
    stream->remote_window_size =
        client->remote_settings.initial_window_size;
    stream->local_window_size = stream->max_local_window_size;

    stream->promised_stream_id = 0;

    stream->save_data_fp = NULL;

    return stream;
}

void
co_http2_stream_destroy(
    co_http2_stream_t* stream
)
{
    if (stream != NULL)
    {
        co_http2_header_destroy(stream->send_header);
        stream->send_header = NULL;

        co_http2_header_destroy(stream->receive_header);
        stream->receive_header = NULL;

        co_byte_array_destroy(stream->header_block_pool.data);
        stream->header_block_pool.data = NULL;

        co_byte_array_destroy(stream->receive_data_pool);
        stream->receive_data_pool = NULL;

        if (stream->receive_data.ptr != NULL)
        {
            co_mem_free(stream->receive_data.ptr);
            stream->receive_data.ptr = NULL;
        }

        if (stream->receive_data.file_path != NULL)
        {
            co_string_destroy(stream->receive_data.file_path);
            stream->receive_data.file_path = NULL;
        }

        if (stream->save_data_fp != NULL)
        {
            fclose(stream->save_data_fp);
            stream->save_data_fp = NULL;
        }

        co_mem_free_later(stream);
    }
}

void
co_http2_stream_update_local_window_size(
    co_http2_stream_t* stream,
    uint32_t consumed_size
)
{
    if (stream->local_window_size > consumed_size)
    {
        stream->local_window_size -= consumed_size;
    }
    else
    {
        stream->local_window_size = 0;
    }

    if ((stream->max_local_window_size * 0.0) >=
        stream->local_window_size)
    {
        if ((stream->max_local_window_size * 2) <
            CO_HTTP2_SETTING_MAX_WINDOW_SIZE)
        {
            stream->max_local_window_size *= 2;
        }
        else
        {
            stream->max_local_window_size =
                CO_HTTP2_SETTING_MAX_WINDOW_SIZE;
        }

        co_http2_frame_t* frame =
            co_http2_create_window_update_frame(
                (stream->max_local_window_size -
                    stream->local_window_size));

        co_http2_stream_send_frame(stream, frame);

        stream->local_window_size = stream->max_local_window_size;
    }
}

static void
co_http2_stream_update_remote_window_size(
    co_http2_stream_t* stream,
    uint32_t consumed_size
)
{
    if (stream->remote_window_size > consumed_size)
    {
        stream->remote_window_size -= consumed_size;
    }
    else
    {
        stream->remote_window_size = 0;
    }
}

static int
co_http2_stream_change_state(
    co_http2_stream_t* stream,
    bool send,
    const co_http2_frame_t* frame
)
{
    if (frame->header.type == CO_HTTP2_FRAME_TYPE_GOAWAY)
    {
        stream->state = CO_HTTP2_STREAM_STATE_CLOSED;

        return CO_HTTP2_STREAM_ERROR_NO_ERROR;
    }

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

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PRIORITY)
        {
            sprintf(info, "dep_id:%u, weight:%u",
                frame->payload.headers.stream_dependency,
                frame->payload.headers.weight);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PRIORITY:
    {
        sprintf(type, "PRIO(%u)", frame->header.type);

        sprintf(info, "dep_id:%u, weight:%u",
            frame->payload.priority.stream_dependency,
            frame->payload.priority.weight);

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

    if (result &&
        (frame->header.type == CO_HTTP2_FRAME_TYPE_DATA))
    {
        co_http2_stream_update_remote_window_size(
            stream->client->system_stream, frame->header.length);
        co_http2_stream_update_remote_window_size(
            stream, frame->header.length);
    }

    co_byte_array_destroy(buffer);

    co_http2_frame_destroy(frame);

    return result;
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
        co_http2_header_print(stream->receive_header);
        printf("[CO_HTTP2] <INF> ======= CONTENT_SIZE(%zu)\n",
            stream->receive_data.size);
    }
#endif

    if (stream->on_message != NULL)
    {
        co_http2_message_fn handler = stream->on_message;

        handler(
            stream->client->tcp_client->sock.owner_thread,
            stream->client, stream,
            stream->receive_header, &stream->receive_data,
            error_code);
    }

    co_mem_free(stream->receive_data.ptr);
    stream->receive_data.ptr = NULL;

    co_string_destroy(stream->receive_data.file_path);
    stream->receive_data.file_path = NULL;
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
        co_http2_stream_send_rst_stream(stream, error_code);

        return false;
    }

    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_DATA:
    {
        uint32_t window_size = co_min(
            stream->local_window_size,
            stream->client->system_stream->local_window_size);

        if (frame->header.length > window_size)
        {
            co_http2_stream_send_rst_stream(
                stream, CO_HTTP2_STREAM_ERROR_FRAME_SIZE_ERROR);

            return false;
        }

        if (stream->receive_header == NULL)
        {
            break;
        }

        co_http2_stream_update_local_window_size(stream, frame->header.length);

        if (stream->save_data_fp != NULL)
        {
            if (frame->payload.data.data_length > 0)
            {
                fwrite(frame->payload.data.data, 1,
                    frame->payload.data.data_length,
                    stream->save_data_fp);

                stream->receive_data.size +=
                    frame->payload.data.data_length;
            }

            if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_STREAM)
            {
                fclose(stream->save_data_fp);
                stream->save_data_fp = NULL;

                co_http2_stream_on_receive_message(stream, 0);
            }
        }
        else if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_STREAM)
        {
            if ((stream->receive_data_pool == NULL) ||
                (co_byte_array_get_count(stream->receive_data_pool) == 0))
            {
                stream->receive_data.ptr =
                    frame->payload.data.data;
                stream->receive_data.size =
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

                    stream->receive_data.size =
                        (uint32_t)co_byte_array_get_count(stream->receive_data_pool) - 1;
                    stream->receive_data.ptr =
                        co_byte_array_detach(stream->receive_data_pool);
                }
                else
                {
                    stream->receive_data.ptr = NULL;
                    stream->receive_data.size = 0;
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
        if (stream->receive_header != NULL)
        {
            break;
        }        

        stream->receive_header = co_http2_header_create();

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PRIORITY)
        {
            stream->receive_header->stream_dependency =
                frame->payload.headers.stream_dependency;
            stream->receive_header->weight =
                frame->payload.headers.weight;
        }

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_HEADERS)
        {
            if (!co_http2_hpack_deserialize_header(
                frame->payload.headers.header_block_fragment,
                frame->payload.headers.header_block_fragment_length,
                &stream->client->local_dynamic_table,
                stream->receive_header))
            {
                co_http2_stream_send_rst_stream(
                    stream, CO_HTTP2_STREAM_ERROR_COMPRESSION_ERROR);

                return false;
            }

            co_byte_array_clear(stream->header_block_pool.data);

            if (stream->state == CO_HTTP2_STREAM_STATE_REMOTE_CLOSED)
            {
                co_http2_stream_on_receive_message(stream, 0);
            }
            else if (stream->send_header == NULL)
            {
                co_http2_stream_on_receive_message(stream, 0);
            }
            else
            {
                if (stream->receive_data.file_path != NULL)
                {
                    stream->save_data_fp =
                        fopen(stream->receive_data.file_path, "wb");

                    if (stream->save_data_fp == NULL)
                    {
                        co_http2_stream_send_rst_stream(
                            stream, CO_HTTP2_STREAM_ERROR_INTERNAL_ERROR);

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
        if (stream->client->on_priority != NULL)
        {
            co_http2_priority_fn handler = stream->client->on_priority;

            handler(
                stream->client->tcp_client->sock.owner_thread,
                stream->client, stream,
                frame->payload.priority.stream_dependency,
                frame->payload.priority.weight);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_RST_STREAM:
    {
        if (stream->client->on_close_stream != NULL)
        {
            co_http2_close_stream_fn handler = stream->client->on_close_stream;

            handler(
                stream->client->tcp_client->sock.owner_thread,
                stream->client, stream,
                frame->payload.rst_stream.error_code);
        }

        if ((frame->payload.rst_stream.error_code !=
                CO_HTTP2_STREAM_ERROR_CANCEL) &&
            (frame->payload.rst_stream.error_code !=
                CO_HTTP2_STREAM_ERROR_REFUSED_STREAM))
        {
            co_http2_stream_on_receive_message(stream,
                (CO_HTTP2_ERROR_STREAM_CLOSED -
                    frame->payload.rst_stream.error_code));
        }
        
        break;
    }
    case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
    {
        if (co_http2_get_stream(stream->client,
            frame->payload.push_promise.promised_stream_id) != NULL)
        {
            return false;
        }

        stream->promised_stream_id = frame->payload.push_promise.promised_stream_id;

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_END_HEADERS)
        {
            co_http2_header_t* header = co_http2_header_create();

            if (!co_http2_hpack_deserialize_header(
                frame->payload.push_promise.header_block_fragment,
                frame->payload.push_promise.header_block_fragment_length,
                &stream->client->local_dynamic_table,
                header))
            {
                co_http2_header_destroy(header);

                co_http2_stream_send_rst_stream(
                    stream, CO_HTTP2_STREAM_ERROR_COMPRESSION_ERROR);

                return false;
            }

            co_http2_client_on_push_promise(
                stream->client, stream, stream->promised_stream_id, header);
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
        if ((stream->remote_window_size +
            frame->payload.window_update.window_size_increment) >
            CO_HTTP2_SETTING_MAX_WINDOW_SIZE)
        {
            co_http2_stream_send_rst_stream(
                stream, CO_HTTP2_STREAM_ERROR_FLOW_CONTROL_ERROR);

            return false;
        }

        stream->remote_window_size +=
            frame->payload.window_update.window_size_increment;

        if (stream->client->on_window_update != NULL)
        {
            co_http2_window_update_fn handler =
                stream->client->on_window_update;

            handler(
                stream->client->tcp_client->sock.owner_thread,
                stream->client, stream);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_CONTINUATION:
    {
        if (stream->receive_header == NULL)
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
            size_t data_size =
                co_byte_array_get_count(stream->header_block_pool.data);

            co_http2_header_t* header;
                
            if (stream->header_block_pool.type == CO_HTTP2_FRAME_TYPE_HEADERS)
            {
                header = stream->receive_header;
            }
            else
            {
                header = co_http2_header_create();
            }

            if (!co_http2_hpack_deserialize_header(
                data, data_size,
                &stream->client->local_dynamic_table,
                header))
            {
                if (stream->header_block_pool.type !=
                    CO_HTTP2_FRAME_TYPE_HEADERS)
                {
                    co_http2_header_destroy(header);
                }

                co_http2_stream_send_rst_stream(
                    stream, CO_HTTP2_STREAM_ERROR_COMPRESSION_ERROR);

                return false;
            }

            if (stream->header_block_pool.type == CO_HTTP2_FRAME_TYPE_PUSH_PROMISE)
            {
                co_http2_client_on_push_promise(
                    stream->client, stream, stream->promised_stream_id, header);

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
        co_http2_stream_send_rst_stream(
            stream, CO_HTTP2_STREAM_ERROR_PROTOCOL_ERROR);

        return false;
    }
    }

    return true;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

ssize_t
co_http2_stream_send_data(
    co_http2_stream_t* stream,
    bool end_stream,
    const void* data,
    uint32_t data_size
)
{
    if (stream->id == 0)
    {
        return -1;
    }

    if (data_size > INT32_MAX)
    {
        return -1;
    }

    uint8_t* data_ptr = (uint8_t*)data;

    uint32_t total_data_size = (uint32_t)data_size;
    uint32_t max_frame_size =
        stream->client->remote_settings.max_frame_size;
    uint32_t window_size =
        co_http2_stream_get_sendable_data_size(stream);

    if (total_data_size > window_size)
    {
        total_data_size -= window_size;
    }

    if (total_data_size > max_frame_size)
    {
        uint32_t index = 0;
        uint32_t frame_size = max_frame_size;

        co_http2_frame_t* data_frame =
            co_http2_create_data_frame(
                false, false, data_ptr, frame_size, NULL, 0);

        co_http2_stream_send_frame(stream, data_frame);

        index += frame_size;

        do
        {
            bool last = false;

            if ((total_data_size - index) > max_frame_size)
            {
                frame_size = max_frame_size;
            }
            else
            {
                frame_size = total_data_size - index;

                last = end_stream;
            }

            data_frame = co_http2_create_data_frame(
                false, last, &data_ptr[index], frame_size,
                NULL, 0);

            co_http2_stream_send_frame(stream, data_frame);

            index += frame_size;

        } while (total_data_size > index);
    }
    else
    {
        co_http2_frame_t* data_frame =
            co_http2_create_data_frame(
                false, end_stream,
                data_ptr, total_data_size, NULL, 0);

        co_http2_stream_send_frame(stream, data_frame);
    }

    return total_data_size;
}

bool
co_http2_stream_send_header(
    co_http2_stream_t* stream,
    bool end_stream,
    co_http2_header_t* header
)
{
    if (stream->id == 0)
    {
        return false;
    }

    stream->send_header = header;

    if (stream->client->base_url != NULL)
    {
        if (header->pseudo.authority == NULL)
        {
            header->pseudo.authority =
                co_http_url_create_host_and_port(stream->client->base_url);
        }

        if (header->pseudo.scheme == NULL)
        {
            header->pseudo.scheme =
                co_string_duplicate(stream->client->base_url->scheme);
        }
    }

#ifdef CO_HTTP2_DEBUG
    printf("[CO_HTTP2] <INF> =========== SEND ============\n");
    co_http2_header_print(header);
    printf("[CO_HTTP2] <INF> =============================\n");
#endif

    co_byte_array_t* request_data = co_byte_array_create();

    co_http2_hpack_serialize_header(
        header, &stream->client->remote_dynamic_table, request_data);

    uint8_t* data_ptr =
        co_byte_array_get_ptr(request_data, 0);
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
                false, end_stream, false, data_ptr, data_size,
                header->stream_dependency, header->weight,
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
                    false, end_headers, &data_ptr[index], data_size);

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
                false, end_stream, true, data_ptr, total_data_size,
                header->stream_dependency, header->weight,
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

co_http2_stream_t*
co_http2_stream_send_server_push_request(
    co_http2_stream_t* stream,
    co_http2_header_t* header
)
{
    if (stream->client->remote_settings.enable_push == 0)
    {
        return NULL;
    }

    if (co_http2_header_get_method(header) == NULL)
    {
        co_http2_header_set_method(header, "GET");
    }

    if (co_http2_header_get_scheme(header) == NULL)
    {
        if (stream->client->tcp_client->sock.tls == NULL)
        {
            co_http2_header_set_scheme(header, "http");
        }
        else
        {
            co_http2_header_set_scheme(header, "https");
        }
    }

#ifdef CO_HTTP2_DEBUG
    printf("[CO_HTTP2] <INF> ======== PUSH PROMISE =======\n");
    co_http2_header_print(header);
    printf("[CO_HTTP2] <INF> =============================\n");
#endif

    co_byte_array_t* request_data = co_byte_array_create();

    co_http2_hpack_serialize_header(
        header, &stream->client->remote_dynamic_table, request_data);

    uint8_t* data_ptr =
        co_byte_array_get_ptr(request_data, 0);
    const uint32_t total_data_size =
        (const uint32_t)co_byte_array_get_count(request_data);

    const uint32_t max_frame_size =
        stream->client->remote_settings.max_frame_size;

    co_http2_stream_t* response_stream =
        co_http2_create_stream(stream->client);

    if (total_data_size > max_frame_size)
    {
        uint32_t index = 0;
        uint32_t data_size = max_frame_size;

        co_http2_frame_t* push_promise_frame =
            co_http2_create_push_promise_frame(
                false, false, response_stream->id,
                data_ptr, total_data_size,
                NULL, 0);

        if (!co_http2_stream_send_frame(stream, push_promise_frame))
        {
            co_byte_array_destroy(request_data);
            co_http2_header_destroy(header);

            return NULL;
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
                    false, end_headers, &data_ptr[index], data_size);

            if (!co_http2_stream_send_frame(stream, continuation_frame))
            {
                co_byte_array_destroy(request_data);
                co_http2_header_destroy(header);

                return NULL;
            }

            index += data_size;

        } while (total_data_size > index);
    }
    else
    {
        co_http2_frame_t* push_promise_frame =
            co_http2_create_push_promise_frame(
                false, true, response_stream->id,
                data_ptr, total_data_size,
                NULL, 0);

        if (!co_http2_stream_send_frame(stream, push_promise_frame))
        {
            co_byte_array_destroy(request_data);
            co_http2_header_destroy(header);

            return NULL;
        }
    }

    co_byte_array_destroy(request_data);
    co_http2_header_destroy(header);

    return response_stream;
}

bool
co_http2_stream_send_window_update(
    co_http2_stream_t* stream,
    uint32_t window_size_increment
)
{
    if ((stream->local_window_size + window_size_increment) >
        CO_HTTP2_SETTING_MAX_WINDOW_SIZE)
    {
        window_size_increment =
            CO_HTTP2_SETTING_MAX_WINDOW_SIZE - stream->local_window_size;
    }

    stream->local_window_size += window_size_increment;

    if (stream->local_window_size > stream->max_local_window_size)
    {
        stream->max_local_window_size = stream->local_window_size;
    }

    co_http2_frame_t* window_update_frame =
        co_http2_create_window_update_frame(window_size_increment);

    return co_http2_stream_send_frame(stream, window_update_frame);
}

bool
co_http2_stream_send_rst_stream(
    co_http2_stream_t* stream,
    uint32_t error_code
)
{
    co_http2_frame_t* rst_frame =
        co_http2_create_rst_stream_frame(error_code);

    return co_http2_stream_send_frame(stream, rst_frame);
}

bool
co_http2_stream_send_priority(
    co_http2_stream_t* stream,
    uint32_t stream_dependency,
    uint8_t weight
)
{
    co_http2_frame_t* priority_frame =
        co_http2_create_priority_frame(stream_dependency, weight);

    return co_http2_stream_send_frame(stream, priority_frame);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

const co_http2_header_t*
co_http2_stream_get_send_header(
    const co_http2_stream_t* stream
)
{
    return stream->send_header;
}

uint32_t
co_http2_stream_get_id(
    const co_http2_stream_t* stream
)
{
    return stream->id;
}

uint32_t
co_http2_stream_get_state(
    const co_http2_stream_t* stream
)
{
    return stream->state;
}

uint32_t
co_http2_stream_get_local_window_size(
    const co_http2_stream_t* stream)
{
    return stream->local_window_size;
}

uint32_t
co_http2_stream_get_remote_window_size(
    const co_http2_stream_t* stream
)
{
    return stream->remote_window_size;
}

uint32_t
co_http2_stream_get_sendable_data_size(
    const co_http2_stream_t* stream
)
{
    if (stream->id == 0)
    {
        return 0;
    }

    return co_min(
        stream->client->system_stream->remote_window_size,
        stream->remote_window_size);
}

void
co_http2_stream_set_save_file_path(
    co_http2_stream_t* stream,
    const char* file_path
)
{
    co_string_destroy(stream->receive_data.file_path);

    if (file_path != NULL)
    {
        stream->receive_data.file_path = co_string_duplicate(file_path);
    }
    else
    {
        stream->receive_data.file_path = NULL;
    }
}
