#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_byte_order.h>

#include <coldforce/http/co_http.h>

#include <coldforce/http2/co_http2_frame.h>
#include <coldforce/http2/co_http2_client.h>

//---------------------------------------------------------------------------//
// http2 frame
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

typedef struct
{
    union co_http2_frame_length_value_t
    {
        uint8_t u8[4];
        uint32_t u32;

    } value;

} co_http2_frame_length_t;

void
co_http2_frame_serialize(
    const co_http2_frame_t* frame,
    co_byte_array_t* buffer
)
{
    uint16_t u16;
    uint32_t u32;

    co_http2_frame_length_t length24;
    length24.value.u32 =
        co_byte_order_32_host_to_network(frame->header.length);

    co_byte_array_add(buffer, &length24.value.u8[1], 3);
    co_byte_array_add(buffer, &frame->header.type, sizeof(uint8_t));
    co_byte_array_add(buffer, &frame->header.flags, sizeof(uint8_t));

    u32 = co_byte_order_32_host_to_network(frame->header.stream_id);
    co_byte_array_add(buffer, &u32, sizeof(uint32_t));

    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_DATA:
    {
        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            co_byte_array_add(buffer,
                &frame->payload.data.pad_length, sizeof(uint8_t));
        }

        co_byte_array_add(buffer,
            frame->payload.data.data, frame->payload.data.data_length);

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            co_byte_array_add(buffer,
                frame->payload.data.padding, frame->payload.data.pad_length);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_HEADERS:
    {
        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            co_byte_array_add(buffer,
                &frame->payload.headers.pad_length, sizeof(uint8_t));
        }

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PRIORITY)
        {
            u32 = co_byte_order_32_host_to_network(
                frame->payload.headers.stream_dependency);
            co_byte_array_add(buffer, &u32, sizeof(uint32_t));

            co_byte_array_add(buffer,
                &frame->payload.headers.weight, sizeof(uint8_t));
        }

        co_byte_array_add(buffer,
            frame->payload.headers.header_block_fragment,
            frame->payload.headers.header_block_fragment_length);

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            co_byte_array_add(buffer,
                frame->payload.headers.padding, 
                frame->payload.headers.pad_length);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PRIORITY:
    {
        u32 = co_byte_order_32_host_to_network(
            frame->payload.priority.stream_dependency);
        co_byte_array_add(buffer, &u32, sizeof(uint32_t));

        co_byte_array_add(buffer,
            &frame->payload.priority.weight, sizeof(uint8_t));

        break;
    }
    case CO_HTTP2_FRAME_TYPE_RST_STREAM:
    {
        u32 = co_byte_order_32_host_to_network(
            frame->payload.rst_stream.error_code);
        co_byte_array_add(buffer, &u32, sizeof(uint32_t));

        break;
    }
    case CO_HTTP2_FRAME_TYPE_SETTINGS:
    {
        for (uint16_t index = 0;
            index < frame->payload.settings.param_count; ++index)
        {
            u16 = co_byte_order_16_host_to_network(
                frame->payload.settings.params[index].id);
            co_byte_array_add(buffer, &u16, sizeof(uint16_t));

            u32 = co_byte_order_32_host_to_network(
                frame->payload.settings.params[index].value);
            co_byte_array_add(buffer, &u32, sizeof(uint32_t));
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
    {
        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            co_byte_array_add(buffer,
                &frame->payload.push_promise.pad_length, sizeof(uint8_t));
        }

        u32 = co_byte_order_32_host_to_network(
            frame->payload.push_promise.promised_stream_id);
        co_byte_array_add(buffer, &u32, sizeof(uint32_t));

        co_byte_array_add(buffer,
            frame->payload.push_promise.header_block_fragment,
            frame->payload.push_promise.header_block_fragment_length);

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            co_byte_array_add(buffer,
                frame->payload.push_promise.padding,
                frame->payload.push_promise.pad_length);
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PING:
    {
        co_byte_array_add(buffer,
            &frame->payload.ping.opaque_data, sizeof(uint64_t));

        break;
    }
    case CO_HTTP2_FRAME_TYPE_GOAWAY:
    {
        u32 = co_byte_order_32_host_to_network(
            frame->payload.goaway.last_stream_id);
        co_byte_array_add(buffer, &u32, sizeof(uint32_t));

        u32 = co_byte_order_32_host_to_network(
            frame->payload.goaway.error_code);
        co_byte_array_add(buffer, &u32, sizeof(uint32_t));

        co_byte_array_add(buffer,
            frame->payload.goaway.additional_debug_data,
            frame->payload.goaway.additional_debug_data_length);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
    {
        u32 = co_byte_order_32_host_to_network(
            frame->payload.window_update.window_size_increment);
        co_byte_array_add(buffer, &u32, sizeof(uint32_t));

        break;
    }
    case CO_HTTP2_FRAME_TYPE_CONTINUATION:
    {
        co_byte_array_add(buffer,
            frame->payload.continuation.header_block_fragment,
            frame->payload.continuation.header_block_fragment_length);

        break;
    }
    default:
    {
        break;
    }
    }
}

int
co_http2_frame_deserialize(
    const co_byte_array_t* data,
    size_t* index,
    uint32_t max_frame_size,
    co_http2_frame_t* frame
)
{
    size_t data_size = co_byte_array_get_count(data) - (*index);

    if (data_size < CO_HTTP2_FRAME_HEADER_SIZE)
    {
        return CO_HTTP_PARSE_MORE_DATA;
    }

    const uint8_t* data_head =
        co_byte_array_get_const_ptr(data, (*index));
    const uint8_t* data_ptr = data_head;

    co_http2_frame_length_t length24;
    length24.value.u32 = 0;

#ifdef CO_LITTLE_ENDIAN
    length24.value.u8[2] = data_ptr[0];
    length24.value.u8[1] = data_ptr[1];
    length24.value.u8[0] = data_ptr[2];
#else
    length24.value.u8[1] = data_ptr[0];
    length24.value.u8[2] = data_ptr[1];
    length24.value.u8[3] = data_ptr[2];
#endif
    frame->header.length = length24.value.u32;

    if (frame->header.length > max_frame_size)
    {
        return CO_HTTP_PARSE_ERROR;
    }

    if (data_size <
        (CO_HTTP2_FRAME_HEADER_SIZE + (size_t)frame->header.length))
    {
        return CO_HTTP_PARSE_MORE_DATA;
    }

    data_ptr += 3;

    frame->header.type = *data_ptr;
    data_ptr += sizeof(uint8_t);

    frame->header.flags = *data_ptr;
    data_ptr += sizeof(uint8_t);

    memcpy(&frame->header.stream_id, data_ptr, sizeof(uint32_t));
    frame->header.stream_id =
        co_byte_order_32_network_to_host(frame->header.stream_id);
    data_ptr += sizeof(uint32_t);

    frame->header.payload_destroy = true;

    switch (frame->header.type)
    {
    case CO_HTTP2_FRAME_TYPE_DATA:
    {
        uint32_t length = frame->header.length;

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            frame->payload.data.pad_length = *data_ptr;
            data_ptr += sizeof(uint8_t);

            length -= (sizeof(uint8_t) + frame->payload.data.pad_length);
        }
        else
        {
            frame->payload.data.pad_length = 0;
        }

        if (length > 0)
        {
            frame->payload.data.data_length = length;
            frame->payload.data.data = co_mem_alloc((size_t)length + 1);
            memcpy(frame->payload.data.data, data_ptr, length);
            frame->payload.data.data[length] = '\0';
            data_ptr += length;
        }
        else
        {
            frame->payload.data.data = NULL;
            frame->payload.data.data_length = 0;
        }

        frame->payload.data.padding = NULL;
        data_ptr += frame->payload.data.pad_length;

        break;
    }
    case CO_HTTP2_FRAME_TYPE_HEADERS:
    {
        uint32_t length = frame->header.length;

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            frame->payload.headers.pad_length = *data_ptr;
            data_ptr += sizeof(uint8_t);

            length -= (sizeof(uint8_t) + frame->payload.headers.pad_length);
        }
        else
        {
            frame->payload.headers.pad_length = 0;
        }

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PRIORITY)
        {
            memcpy(&frame->payload.headers.stream_dependency,
                data_ptr, sizeof(uint32_t));
            frame->payload.headers.stream_dependency =
                co_byte_order_32_network_to_host(
                    frame->payload.headers.stream_dependency);
            data_ptr += sizeof(uint32_t);
            length -= sizeof(uint32_t);

            frame->payload.headers.weight = *data_ptr;
            data_ptr += sizeof(uint8_t);
            length -= sizeof(uint8_t);
        }
        else
        {
            frame->payload.headers.stream_dependency = 0;
            frame->payload.headers.weight = 0;
        }

        if (length > 0)
        {
            frame->payload.headers.header_block_fragment_length = length;
            frame->payload.headers.header_block_fragment = co_mem_alloc(length);
            memcpy(frame->payload.headers.header_block_fragment,
                data_ptr, length);
            data_ptr += length;
        }
        else
        {
            frame->payload.headers.header_block_fragment = NULL;
            frame->payload.headers.header_block_fragment_length = 0;
        }
        
        frame->payload.headers.padding = NULL;
        data_ptr += frame->payload.headers.pad_length;

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PRIORITY:
    {
        memcpy(&frame->payload.priority.stream_dependency,
            data_ptr, sizeof(uint32_t));
        frame->payload.priority.stream_dependency =
            co_byte_order_32_network_to_host(
                frame->payload.priority.stream_dependency);
        data_ptr += sizeof(uint32_t);

        frame->payload.priority.weight = *data_ptr;
        data_ptr += sizeof(uint8_t);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_RST_STREAM:
    {
        memcpy(&frame->payload.rst_stream.error_code,
            data_ptr, sizeof(uint32_t));
        frame->payload.rst_stream.error_code =
            co_byte_order_32_network_to_host(
                frame->payload.rst_stream.error_code);
        data_ptr += sizeof(uint32_t);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_SETTINGS:
    {
        uint32_t length = frame->header.length;

        frame->payload.settings.param_count =
            (uint16_t)(length / (sizeof(uint16_t) + sizeof(uint32_t)));

        if (frame->payload.settings.param_count > 0)
        {
            frame->payload.settings.params =
                (co_http2_setting_param_st*)
                    co_mem_alloc(sizeof(co_http2_setting_param_st) *
                        frame->payload.settings.param_count);
        
            uint16_t u16;
            uint32_t u32;

            for (uint16_t param_index = 0;
                param_index < frame->payload.settings.param_count;
                ++param_index)
            {
                memcpy(&u16, data_ptr, sizeof(uint16_t));
                frame->payload.settings.params[param_index].id =
                    co_byte_order_16_network_to_host(u16);
                data_ptr += sizeof(uint16_t);

                memcpy(&u32, data_ptr, sizeof(uint32_t));
                frame->payload.settings.params[param_index].value =
                    co_byte_order_32_network_to_host(u32);
                data_ptr += sizeof(uint32_t);
            }
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
    {
        uint32_t length = frame->header.length;

        if (frame->header.flags & CO_HTTP2_FRAME_FLAG_PADDED)
        {
            frame->payload.push_promise.pad_length = *data_ptr;
            data_ptr += sizeof(uint8_t);

            length -= (sizeof(uint8_t) +
                frame->payload.push_promise.pad_length);
        }
        else
        {
            frame->payload.push_promise.pad_length = 0;
        }

        memcpy(&frame->payload.push_promise.promised_stream_id,
            data_ptr, sizeof(uint32_t));
        frame->payload.push_promise.promised_stream_id =
            co_byte_order_32_network_to_host(
                frame->payload.push_promise.promised_stream_id);
        data_ptr += sizeof(uint32_t);
        length -= sizeof(uint32_t);

        frame->payload.push_promise.header_block_fragment_length = length;
        frame->payload.push_promise.header_block_fragment =
            co_mem_alloc(length);
        memcpy(frame->payload.push_promise.header_block_fragment,
            data_ptr, length);
        data_ptr += length;

        frame->payload.push_promise.padding = NULL;
        data_ptr += frame->payload.push_promise.pad_length;
        
        break;
    }
    case CO_HTTP2_FRAME_TYPE_PING:
    {
        memcpy(&frame->payload.ping.opaque_data,
            data_ptr, sizeof(uint64_t));
        data_ptr += sizeof(uint64_t);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_GOAWAY:
    {
        memcpy(&frame->payload.goaway.last_stream_id,
            data_ptr, sizeof(uint32_t));
        frame->payload.goaway.last_stream_id =
            co_byte_order_32_network_to_host(
                frame->payload.goaway.last_stream_id);
        data_ptr += sizeof(uint32_t);

        memcpy(&frame->payload.goaway.error_code,
            data_ptr, sizeof(uint32_t));
        frame->payload.goaway.error_code =
            co_byte_order_32_network_to_host(
                frame->payload.goaway.error_code);
        data_ptr += sizeof(uint32_t);

        uint32_t length = frame->header.length
            - sizeof(uint32_t) - sizeof(uint32_t);

        frame->payload.goaway.additional_debug_data_length = length;
        frame->payload.goaway.additional_debug_data = NULL;

        if (length > 0)
        {
            frame->payload.goaway.additional_debug_data =
                co_mem_alloc(length);
            memcpy(frame->payload.goaway.additional_debug_data,
                data_ptr, length);
            data_ptr += length;
        }

        break;
    }
    case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
    {
        memcpy(&frame->payload.window_update.window_size_increment,
            data_ptr, sizeof(uint32_t));
        frame->payload.window_update.window_size_increment =
            co_byte_order_32_network_to_host(
                frame->payload.window_update.window_size_increment);
        data_ptr += sizeof(uint32_t);

        break;
    }
    case CO_HTTP2_FRAME_TYPE_CONTINUATION:
    {
        uint32_t length = frame->header.length;

        frame->payload.continuation.header_block_fragment_length = length;
        frame->payload.continuation.header_block_fragment =
            co_mem_alloc(length);
        memcpy(frame->payload.continuation.header_block_fragment,
            data_ptr, length);
        data_ptr += length;

        break;
    }
    default:
    {
        return CO_HTTP_PARSE_ERROR;
    }
    }

    (*index) += (data_ptr - data_head);

    return CO_HTTP_PARSE_COMPLETE;
}

co_http2_frame_t*
co_http2_frame_create(
    void
)
{
    co_http2_frame_t* frame =
        (co_http2_frame_t*)co_mem_alloc(sizeof(co_http2_frame_t));

    if (frame != NULL)
    {
        memset(frame, 0x00, sizeof(co_http2_frame_t));
    }

    return frame;
}

void
co_http2_frame_destroy(
    co_http2_frame_t* frame
)
{
    if (frame != NULL)
    {
        if (frame->header.payload_destroy)
        {
            switch (frame->header.type)
            {
            case CO_HTTP2_FRAME_TYPE_DATA:
            {
                co_mem_free(frame->payload.data.data);
                co_mem_free(frame->payload.data.padding);

                break;
            }
            case CO_HTTP2_FRAME_TYPE_HEADERS:
            {
                co_mem_free(frame->payload.headers.header_block_fragment);
                co_mem_free(frame->payload.headers.padding);

                break;
            }
            case CO_HTTP2_FRAME_TYPE_PRIORITY:
            {
                break;
            }
            case CO_HTTP2_FRAME_TYPE_RST_STREAM:
            {
                break;
            }
            case CO_HTTP2_FRAME_TYPE_SETTINGS:
            {
                co_mem_free(frame->payload.settings.params);

                break;
            }
            case CO_HTTP2_FRAME_TYPE_PUSH_PROMISE:
            {
                co_mem_free(frame->payload.push_promise.header_block_fragment);
                co_mem_free(frame->payload.push_promise.padding);

                break;
            }
            case CO_HTTP2_FRAME_TYPE_PING:
            {
                break;
            }
            case CO_HTTP2_FRAME_TYPE_GOAWAY:
            {
                co_mem_free(frame->payload.goaway.additional_debug_data);

                break;
            }
            case CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE:
            {
                break;
            }
            case CO_HTTP2_FRAME_TYPE_CONTINUATION:
            {
                co_mem_free(frame->payload.continuation.header_block_fragment);

                break;
            }
            default:
            {
                break;
            }
            }
        }

        co_mem_free(frame);
    }
}

co_http2_frame_t*
co_http2_create_data_frame(
    bool payload_copy,
    bool end_stream,
    const uint8_t* data,
    uint32_t data_length,
    const uint8_t* padding,
    uint8_t pad_length
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = data_length;
    frame->header.type = CO_HTTP2_FRAME_TYPE_DATA;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->header.payload_destroy = payload_copy;

    if (end_stream)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_END_STREAM;
    }

    frame->payload.data.data_length = data_length;

    if (frame->payload.data.data_length > 0)
    {
        if (payload_copy)
        {
            frame->payload.data.data =
                (uint8_t*)co_mem_alloc(frame->payload.data.data_length);
            memcpy(frame->payload.data.data,
                data, frame->payload.data.data_length);
        }
        else
        {
            frame->payload.data.data = (uint8_t*)data;
        }
    }
    else
    {
        frame->payload.data.data = NULL;
    }

    frame->payload.data.pad_length = pad_length;

    if (pad_length > 0)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_PADDED;

        if (payload_copy)
        {
            frame->payload.data.padding =
                (uint8_t*)co_mem_alloc(pad_length);
            memcpy(frame->payload.data.padding, padding, pad_length);
        }
        else
        {
            frame->payload.data.padding = (uint8_t*)padding;
        }

        frame->header.length += pad_length;
    }
    else
    {
        frame->payload.data.padding = NULL;
    }

    return frame;
}

co_http2_frame_t*
co_http2_create_headers_frame(
    bool payload_copy,
    bool end_stream,
    bool end_headers,
    const uint8_t* header_block_fragment,
    uint32_t header_block_fragment_length,
    uint32_t stream_dependency,
    uint8_t weight,
    const uint8_t* padding,
    uint8_t pad_length
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = header_block_fragment_length;
    frame->header.type = CO_HTTP2_FRAME_TYPE_HEADERS;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->header.payload_destroy = payload_copy;

    if (end_stream)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_END_STREAM;
    }

    if (end_headers)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_END_HEADERS;
    }

    frame->payload.headers.header_block_fragment_length =
        header_block_fragment_length;

    if (header_block_fragment_length > 0)
    {
        if (payload_copy)
        {
            frame->payload.headers.header_block_fragment =
                (uint8_t*)co_mem_alloc(header_block_fragment_length);
            memcpy(frame->payload.headers.header_block_fragment,
                header_block_fragment, header_block_fragment_length);
        }
        else
        {
            frame->payload.headers.header_block_fragment =
                (uint8_t*)header_block_fragment;
        }
    }
    else
    {
        frame->payload.headers.header_block_fragment = NULL;
    }
    
    if ((stream_dependency != 0) || (weight != 0))
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_PRIORITY;

        frame->payload.headers.stream_dependency = stream_dependency;
        frame->payload.headers.weight = weight;
        frame->header.length += (sizeof(uint8_t) + sizeof(uint32_t));
    }

    frame->payload.headers.pad_length = pad_length;

    if (pad_length > 0)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_PADDED;

        if (payload_copy)
        {
            frame->payload.headers.padding =
                (uint8_t*)co_mem_alloc(pad_length);
            memcpy(frame->payload.headers.padding, padding, pad_length);
        }
        else
        {
            frame->payload.headers.padding = (uint8_t*)padding;
        }

        frame->header.length += pad_length;
    }
    else
    {
        frame->payload.headers.padding = NULL;
    }

    return frame;
}

co_http2_frame_t*
co_http2_create_priority_frame(
    uint32_t stream_dependency,
    uint8_t weight
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = sizeof(uint32_t) + sizeof(uint8_t);

    frame->header.type = CO_HTTP2_FRAME_TYPE_PRIORITY;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->payload.priority.stream_dependency = stream_dependency;
    frame->payload.priority.weight = weight;

    return frame;
}

co_http2_frame_t*
co_http2_create_rst_stream_frame(
    uint32_t error_code
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = sizeof(uint32_t);

    frame->header.type = CO_HTTP2_FRAME_TYPE_RST_STREAM;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->payload.rst_stream.error_code = error_code;

    return frame;
}

co_http2_frame_t*
co_http2_create_settings_frame(
    bool payload_copy,
    bool ack,
    const co_http2_setting_param_st* params,
    uint16_t param_count
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length =
        (sizeof(uint16_t) + sizeof(uint32_t)) * param_count;
    frame->header.type = CO_HTTP2_FRAME_TYPE_SETTINGS;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->header.payload_destroy = payload_copy;

    if (ack)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_ACK;
    }

    frame->payload.settings.param_count = param_count;

    if (param_count > 0)
    {
        if (payload_copy)
        {
            frame->payload.settings.params =
                (co_http2_setting_param_st*)co_mem_alloc(
                    sizeof(co_http2_setting_param_st) * param_count);

            for (uint16_t index = 0; index < param_count; ++index)
            {
                frame->payload.settings.params[index].id =
                    params[index].id;
                frame->payload.settings.params[index].value =
                    params[index].value;
            }
        }
        else
        {
            frame->payload.settings.params =
                (co_http2_setting_param_st*)params;
        }
    }
    else
    {
        frame->payload.settings.params = NULL;
    }

    return frame;
}

co_http2_frame_t*
co_http2_create_push_promise_frame(
    bool payload_copy,
    bool end_headers,
    uint32_t promised_stream_id,
    const uint8_t* header_block_fragment,
    uint32_t header_block_fragment_length,
    const uint8_t* padding,
    uint8_t pad_length
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = sizeof(uint32_t) + header_block_fragment_length;
    frame->header.type = CO_HTTP2_FRAME_TYPE_PUSH_PROMISE;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->header.payload_destroy = payload_copy;

    if (end_headers)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_END_HEADERS;
    }

    frame->payload.push_promise.promised_stream_id = promised_stream_id;

    frame->payload.push_promise.header_block_fragment_length =
        header_block_fragment_length;

    if (header_block_fragment_length > 0)
    {
        if (payload_copy)
        {
            frame->payload.push_promise.header_block_fragment =
                (uint8_t*)co_mem_alloc(header_block_fragment_length);
            memcpy(frame->payload.push_promise.header_block_fragment,
                header_block_fragment, header_block_fragment_length);
        }
        else
        {
            frame->payload.push_promise.header_block_fragment =
                (uint8_t*)header_block_fragment;
        }
    }
    else
    {
        frame->payload.push_promise.header_block_fragment = NULL;
    }

    frame->payload.push_promise.pad_length = pad_length;

    if (pad_length > 0)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_PADDED;

        if (payload_copy)
        {
            frame->payload.push_promise.padding =
                (uint8_t*)co_mem_alloc(pad_length);
            memcpy(frame->payload.push_promise.padding, padding, pad_length);
        }
        else
        {
            frame->payload.push_promise.padding = (uint8_t*)padding;
        }

        frame->header.length += pad_length;
    }
    else
    {
        frame->payload.push_promise.padding = NULL;
    }

    return frame;
}

co_http2_frame_t*
co_http2_create_ping_frame(
    bool ack,
    uint64_t opaque_data
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = sizeof(uint64_t);

    frame->header.type = CO_HTTP2_FRAME_TYPE_PING;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    if (ack)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_ACK;
    }

    frame->payload.ping.opaque_data = opaque_data;

    return frame;
}

co_http2_frame_t*
co_http2_create_goaway_frame(
    bool payload_copy,
    uint32_t last_stream_id,
    uint32_t error_code,
    const uint8_t* additional_debug_data,
    uint32_t additional_debug_data_length)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = sizeof(uint32_t) + sizeof(uint32_t);

    frame->header.type = CO_HTTP2_FRAME_TYPE_GOAWAY;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->header.payload_destroy = payload_copy;

    frame->payload.goaway.last_stream_id = last_stream_id;
    frame->payload.goaway.error_code = error_code;

    frame->payload.goaway.additional_debug_data_length =
        additional_debug_data_length;

    if ((additional_debug_data != NULL) &&
        (additional_debug_data_length > 0))
    {
        frame->header.length += additional_debug_data_length;

        if (payload_copy)
        {
            frame->payload.goaway.additional_debug_data =
                (uint8_t*)co_mem_alloc(additional_debug_data_length);
            memcpy(frame->payload.goaway.additional_debug_data,
                additional_debug_data,
                additional_debug_data_length);
        }
        else
        {
            frame->payload.goaway.additional_debug_data =
                (uint8_t*)additional_debug_data;
        }
    }
    else
    {
        frame->payload.goaway.additional_debug_data = NULL;
    }

    return frame;
}

co_http2_frame_t*
co_http2_create_window_update_frame(
    uint32_t window_size_increment
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = sizeof(uint32_t);

    frame->header.type = CO_HTTP2_FRAME_TYPE_WINDOW_UPDATE;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->payload.window_update.window_size_increment = window_size_increment;

    return frame;
}

co_http2_frame_t*
co_http2_create_continuation_frame(
    bool payload_copy,
    bool end_headers,
    const uint8_t* header_block_fragment,
    uint32_t header_block_fragment_length
)
{
    co_http2_frame_t* frame = co_http2_frame_create();

    frame->header.length = header_block_fragment_length;
    frame->header.type = CO_HTTP2_FRAME_TYPE_CONTINUATION;
    frame->header.flags = 0;
    frame->header.stream_id = 0;

    frame->header.payload_destroy = payload_copy;

    if (end_headers)
    {
        frame->header.flags |= CO_HTTP2_FRAME_FLAG_END_HEADERS;
    }

    frame->payload.continuation.header_block_fragment_length =
        header_block_fragment_length;

    if (header_block_fragment_length > 0)
    {
        if (payload_copy)
        {
            frame->payload.continuation.header_block_fragment =
                (uint8_t*)co_mem_alloc(header_block_fragment_length);
            memcpy(frame->payload.continuation.header_block_fragment,
                header_block_fragment, header_block_fragment_length);
        }
        else
        {
            frame->payload.continuation.header_block_fragment =
                (uint8_t*)header_block_fragment;
        }
    }
    else
    {
        frame->payload.continuation.header_block_fragment = NULL;
    }

    return frame;
}
