#ifndef CO_WS_FRAME_H_INCLUDED
#define CO_WS_FRAME_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket frame
//---------------------------------------------------------------------------//

#define CO_WS_FRAME_HEADER_MIN_SIZE    2
#define CO_WS_FRAME_HEADER_MAX_SIZE    16
#define CO_WS_FRAME_MASK_SIZE          4

#define CO_WS_CLOSE_REASON_NORMAL               1000
#define CO_WS_CLOSE_REASON_GOING_AWAY           1001
#define CO_WS_CLOSE_REASON_PROTOCOL_ERROR       1002
#define CO_WS_CLOSE_REASON_DATA_TYPE_ERROR      1003
#define CO_WS_CLOSE_REASON_MESSAGE_TYPE_ERROR   1007
#define CO_WS_CLOSE_REASON_POLICY_VIOLATION     1008
#define CO_WS_CLOSE_REASON_DATA_TOO_BIG         1009
#define CO_WS_CLOSE_REASON_NEGOTIATION_ERROR    1010
#define CO_WS_CLOSE_REASON_UNEXPECTED_ERROR     1011

#define CO_WS_OPCODE_CONTINUATION   0x00
#define CO_WS_OPCODE_TEXT           0x01
#define CO_WS_OPCODE_BINARY         0x02

#define CO_WS_OPCODE_CLOSE          0x08
#define CO_WS_OPCODE_PING           0x09
#define CO_WS_OPCODE_PONG           0x0a

typedef struct
{
    bool fin;
    uint8_t opcode;
    uint64_t payload_size;

} co_ws_frame_header_t;

typedef struct
{
    co_ws_frame_header_t header;
    uint8_t* payload_data;

} co_ws_frame_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_WS_API bool
co_ws_frame_serialize(
    bool fin,
    uint8_t opcode,
    bool mask,
    const void* data,
    size_t data_size,
    co_byte_array_t* buffer
);

CO_WS_API int
co_ws_frame_deserialize(
    co_ws_frame_t* frame,
    const uint8_t* data,
    const size_t data_size,
    size_t* index
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_API
co_ws_frame_t*
co_ws_frame_create(
    void
);

CO_WS_API
void
co_ws_frame_destroy(
    co_ws_frame_t* frame
);

CO_WS_API
bool
co_ws_frame_get_fin(
    const co_ws_frame_t* frame
);

CO_WS_API
uint8_t
co_ws_frame_get_opcode(
    const co_ws_frame_t* frame
);

CO_WS_API
uint64_t
co_ws_frame_get_payload_size(
    const co_ws_frame_t* frame
);

CO_WS_API
const uint8_t*
co_ws_frame_get_payload_data(
    const co_ws_frame_t* frame
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_FRAME_H_INCLUDED
