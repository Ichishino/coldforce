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

void co_http2_hpack_serialize_int(
    uint8_t bit_max, uint8_t flag, uint32_t value, co_byte_array_t* buffer);
bool co_http2_hpack_deserialize_int(
    uint8_t bit_max, const uint8_t* data, size_t data_size,
    uint32_t* value, size_t* index);

void co_http2_hpack_serialize_4bits_int(
    bool flag, uint32_t value, co_byte_array_t* buffer);
void co_http2_hpack_serialize_5bits_int(
    bool flag, uint32_t value, co_byte_array_t* buffer);
void co_http2_hpack_serialize_6bits_int(
    bool flag, uint32_t value, co_byte_array_t* buffer);
void co_http2_hpack_serialize_7bits_int(
    bool flag, uint32_t value, co_byte_array_t* buffer);

void co_http2_hpack_serialize_string(
    bool encoding, const char* str, uint32_t str_length,
    co_byte_array_t* buffer);
bool co_http2_hpack_deserialize_string(
    const uint8_t* data, size_t data_size,
    char** str, uint32_t* str_length,
    size_t* index);

void co_http2_hpack_dynamic_table_setup(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t max_size);
void co_http2_hpack_dynamic_table_cleanup(
    co_http2_hpack_dynamic_table_t* dynamic_table);
void co_http2_hpack_dynamic_table_resize(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t max_size);
bool co_http2_hpack_dynamic_table_add_item(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    const char* name, const char* value);
bool co_http2_hpack_dynamic_table_find_item(
    const co_http2_hpack_dynamic_table_t* dynamic_table,
    const char* name, const char* value, uint32_t* header_index);
bool co_http2_hpack_dynamic_table_find_item_by_index(
    const co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t header_index,
    char** name, char** value);

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
