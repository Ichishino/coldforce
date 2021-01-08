#ifndef CO_HTTP2_HPACK_H_INCLUDED
#define CO_HTTP2_HPACK_H_INCLUDED

#include <coldforce/core/co_list.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/http2/co_http2.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 hpack
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http2_stream_t;
struct co_http2_pseudo_header_t;
struct co_http2_header_t;
struct co_http2_message_t;

#define CO_HTTP2_MAX_4_BITS     ((uint8_t)15)
#define CO_HTTP2_MAX_5_BITS     ((uint8_t)31)
#define CO_HTTP2_MAX_6_BITS     ((uint8_t)63)
#define CO_HTTP2_MAX_7_BITS     ((uint8_t)127)

#define CO_HTTP2_FLAG_4_BITS    ((uint8_t)16)
#define CO_HTTP2_FLAG_5_BITS    ((uint8_t)32)
#define CO_HTTP2_FLAG_6_BITS    ((uint8_t)64)
#define CO_HTTP2_FLAG_7_BITS    ((uint8_t)128)

typedef struct
{
    uint32_t size;

    char* name;
    char* value;

} co_http2_hpack_dynamic_table_item_t;

typedef struct
{
    uint32_t max_size;
    uint32_t total_size;

    co_list_t* items;

} co_http2_hpack_dynamic_table_t;

void co_http2_hpack_dynamic_table_setup(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t max_size);

void co_http2_hpack_dynamic_table_cleanup(
    co_http2_hpack_dynamic_table_t* dynamic_table);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_http2_hpack_serialize_header(
    const struct co_http2_header_t* header,
    co_http2_hpack_dynamic_table_t* dynamic_table,
    co_byte_array_t* buffer);

bool co_http2_hpack_deserialize_header(
    const uint8_t* data, size_t data_size,
    co_http2_hpack_dynamic_table_t* dynamic_table,
    struct co_http2_header_t* header);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_HPACK_H_INCLUDED
