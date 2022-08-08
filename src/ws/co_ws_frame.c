#include <coldforce/core/co_std.h>
#include <coldforce/core/co_byte_array.h>
#include <coldforce/core/co_random.h>

#include <coldforce/net/co_byte_order.h>

#include <coldforce/ws/co_ws_frame.h>
#include <coldforce/ws/co_ws_config.h>

//---------------------------------------------------------------------------//
// websocket frame
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_ws_frame_serialize(
    bool fin,
    uint8_t opcode,
    bool mask,
    const void* data,
    size_t data_size,
    co_byte_array_t* buffer
)
{
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

        co_byte_array_add(buffer, header, header_size);

        if (data_size > 0)
        {
            co_byte_array_add(buffer, masked_data, data_size);
        }

        co_mem_free(masked_data);
    }
    else
    {
        co_byte_array_add(buffer, header, header_size);

        if (data_size > 0)
        {
            co_byte_array_add(buffer, data, data_size);
        }
    }

    return true;
}

int
co_ws_frame_deserialize(
    co_ws_frame_t* frame,
    const uint8_t* data,
    const size_t data_size,
    size_t* index
)
{
    size_t temp_index = (*index);

    uint8_t opcode;

    memcpy(&opcode, &data[temp_index], sizeof(opcode));
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

    memcpy(&u8_length, &data[temp_index], sizeof(u8_length));
    temp_index += sizeof(u8_length);

    bool mask = ((u8_length & 0x80) == 0x80);
    u8_length &= 0x7f;

    if (u8_length <= 125)
    {
        frame->header.payload_size = u8_length;
    }
    else if (u8_length == 126)
    {
        if ((data_size - temp_index) < sizeof(uint16_t))
        {
            return CO_WS_PARSE_MORE_DATA;
        }

        uint16_t u16_length;

        memcpy(&u16_length, &data[temp_index], sizeof(u16_length));
        temp_index += sizeof(u16_length);

        frame->header.payload_size =
            co_byte_order_16_network_to_host(u16_length);
    }
    else if (u8_length == 127)
    {
        if ((data_size - temp_index) < sizeof(uint64_t))
        {
            return CO_WS_PARSE_MORE_DATA;
        }

        uint64_t u64_length;

        memcpy(&u64_length, &data[temp_index], sizeof(u64_length));
        temp_index += sizeof(u64_length);

        frame->header.payload_size =
            co_byte_order_64_network_to_host(u64_length);
    }

    uint8_t mask_key[CO_WS_FRAME_MASK_SIZE];

    if (mask)
    {
        if ((data_size - temp_index) < CO_WS_FRAME_MASK_SIZE)
        {
            return CO_WS_PARSE_MORE_DATA;
        }

        memcpy(mask_key, &data[temp_index], sizeof(mask_key));
        temp_index += sizeof(mask_key);
    }

    if ((data_size - temp_index) < frame->header.payload_size)
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

        memcpy(frame->payload_data,
            &data[temp_index], (size_t)frame->header.payload_size);
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
// public
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
