#include <coldforce/core/co_std.h>

#include <coldforce/net/co_byte_order.h>

#include <coldforce/ws/co_ws_frame.h>
#include <coldforce/ws/co_ws_config.h>

//---------------------------------------------------------------------------//
// websocket frame
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

int
co_ws_frame_deserialize(
    co_ws_frame_t* frame,
    const co_byte_array_t* data,
    size_t* index
)
{
    size_t temp_index = (*index);

    uint8_t opcode;

    co_byte_array_get(data, temp_index, &opcode, sizeof(opcode));
    temp_index += sizeof(opcode);

    frame->header.fin = ((opcode & 0x80) == 0x80);
    frame->header.opcode = opcode & 0x7f;

    if (frame->header.opcode > 0x0f)
    {
        return CO_WS_ERROR_INVALID_FRAME;
    }

    frame->header.payload_size = 0;
    frame->payload_data = NULL;

    uint8_t u8_length;

    co_byte_array_get(
        data, temp_index, &u8_length, sizeof(u8_length));
    temp_index += sizeof(u8_length);

    bool mask = ((u8_length & 0x80) == 0x80);
    u8_length &= 0x7f;

    if (u8_length <= 125)
    {
        frame->header.payload_size = u8_length;
    }
    else if (u8_length == 126)
    {
        if ((co_byte_array_get_count(data) - temp_index) < sizeof(uint16_t))
        {
            return CO_WS_PARSE_MORE_DATA;
        }

        uint16_t u16_length;

        co_byte_array_get(
            data, temp_index, &u16_length, sizeof(u16_length));
        temp_index += sizeof(u16_length);

        frame->header.payload_size =
            co_byte_order_16_network_to_host(u16_length);
    }
    else if (u8_length == 127)
    {
        if ((co_byte_array_get_count(data) - temp_index) < sizeof(uint64_t))
        {
            return CO_WS_PARSE_MORE_DATA;
        }

        uint64_t u64_length;

        co_byte_array_get(
            data, temp_index, &u64_length, sizeof(u64_length));
        temp_index += sizeof(u64_length);

        frame->header.payload_size =
            co_byte_order_64_network_to_host(u64_length);
    }

    uint8_t mask_key[CO_WS_FRAME_MASK_SIZE];

    if (mask)
    {
        if ((co_byte_array_get_count(data) - temp_index) <
            CO_WS_FRAME_MASK_SIZE)
        {
            return CO_WS_PARSE_MORE_DATA;
        }

        co_byte_array_get(
            data, temp_index, mask_key, sizeof(mask_key));
        temp_index += sizeof(mask_key);
    }

    if ((co_byte_array_get_count(data) - temp_index) <
        frame->header.payload_size)
    {
        return CO_WS_PARSE_MORE_DATA;
    }

    if ((frame->header.payload_size >
            co_ws_config_get_max_receive_payload_size()) ||
        (frame->header.payload_size > SIZE_MAX))
    {
        return CO_WS_ERROR_DATA_TOO_BIG;
    }
    else if (frame->header.payload_size > 0)
    {
        frame->payload_data =
            (uint8_t*)co_mem_alloc(
                (size_t)frame->header.payload_size + 1);

        if (frame->payload_data == NULL)
        {
            return CO_WS_ERROR_OUT_OF_MEMORY;
        }

        frame->payload_data[frame->header.payload_size] = '\0';

        co_byte_array_get(data, temp_index,
            frame->payload_data, (size_t)frame->header.payload_size);
        temp_index += (size_t)frame->header.payload_size;
    }

    if (mask)
    {
        for (uint64_t payload_index = 0;
            payload_index < frame->header.payload_size;
            ++payload_index)
        {
            frame->payload_data[payload_index] =
                frame->payload_data[payload_index] ^
                    mask_key[payload_index % CO_WS_FRAME_MASK_SIZE];
        }
    }

    (*index) = temp_index;

    return CO_WS_PARSE_COMPLETE;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_ws_frame_t*
co_ws_frame_create(
    void
)
{
    co_ws_frame_t* frame =
        (co_ws_frame_t*)co_mem_alloc(sizeof(co_ws_frame_t));

    if (frame == NULL)
    {
        return NULL;
    }

    frame->header.fin = false;
    frame->header.opcode = 0xff;
    frame->header.payload_size = 0;
    frame->payload_data = NULL;

    return frame;
}

void
co_ws_frame_destroy(
    co_ws_frame_t* frame
)
{
    if (frame != NULL)
    {
        if (frame->payload_data != NULL)
        {
            co_mem_free(frame->payload_data);
        }

        co_mem_free(frame);
    }
}

uint8_t
co_ws_frame_get_opcode(
    const co_ws_frame_t* frame
)
{
    return frame->header.opcode;
}

bool
co_ws_frame_get_fin(
    const co_ws_frame_t* frame
)
{
    return frame->header.fin;
}

uint64_t
co_ws_frame_get_payload_size(
    const co_ws_frame_t* frame
)
{
    return frame->header.payload_size;
}

const uint8_t*
co_ws_frame_get_payload_data(
    const co_ws_frame_t* frame
)
{
    return frame->payload_data;
}
