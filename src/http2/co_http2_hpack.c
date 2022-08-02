#include <coldforce/core/co_std.h>
#include <coldforce/core/co_list.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http2/co_http2_hpack.h>
#include <coldforce/http2/co_http2_huffman.h>
#include <coldforce/http2/co_http2_stream.h>
#include <coldforce/http2/co_http2_client.h>

//---------------------------------------------------------------------------//
// http2 hpack
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static const char* hpack_static_name_table[61] =
{
    ":authority", ":method", ":method", ":path",
    ":path", ":scheme", ":scheme", ":status",
    ":status", ":status", ":status", ":status",
    ":status", ":status", "accept-charset", "accept-encoding",
    "accept-language", "accept-ranges", "accept", "access-control-allow-origin",
    "age", "allow", "authorization", "cache-control",
    "content-disposition", "content-encoding", "content-language", "content-length",
    "content-location", "content-range", "content-type", "cookie",
    "date", "etag", "expect", "expires",
    "from", "host", "if-match", "if-modified-since",
    "if-none-match", "if-range", "if-unmodified-since", "last-modified",
    "link", "location", "max-forwards", "proxy-authenticate",
    "proxy-authorization", "range", "referer", "refresh",
    "retry-after", "server", "set-cookie", "strict-transport-security",
    "transfer-encoding", "user-agent", "vary", "via",
    "www-authenticate"
};

static const char* hpack_static_value_table[16] =
{
    NULL, "GET", "POST", "/",
    "/index.html", "http", "https", "200",
    "204", "206", "304", "400",
    "404", "500", NULL, "gzip, deflate"
};

static const uint8_t hpack_static_hash_name_table[288] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0x33, 0xff, 0x37,
    0x15, 0x22, 0xff, 0xff, 0x39, 0xff, 0x2f, 0x2e,
    0x0f, 0xff, 0x05, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0x23, 0x1d,
    0x2d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x10, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3c, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x36,
    0x13, 0xff, 0xff, 0xff, 0x2b, 0xff, 0xff, 0xff,
    0xff, 0x11, 0xff, 0xff, 0x12, 0x3a, 0xff, 0xff,
    0xff, 0x28, 0xff, 0xff, 0xff, 0xff, 0x31, 0x38,
    0xff, 0xff, 0xff, 0xff, 0x2a, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x3b, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x27, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x16, 0xff, 0xff, 0xff, 0x25, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x24, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x03, 0xff,
    0xff, 0xff, 0xff, 0x19, 0x29, 0x1a, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x35,
    0xff, 0xff, 0xff, 0xff, 0x34, 0xff, 0xff, 0x26,
    0x32, 0xff, 0xff, 0xff, 0xff, 0xff, 0x2c, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0e, 0xff, 0xff,
    0x14, 0x17, 0xff, 0x1e, 0x1c, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x18, 0x20, 0x00, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1b, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x21, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static void
co_http2_hpack_serialize_int(
    uint8_t bit_max,
    uint8_t flag,
    uint32_t value,
    co_byte_array_t* buffer
)
{
    if (value < bit_max)
    {
        uint8_t u8 = ((uint8_t)value) | flag;

        co_byte_array_add(buffer, &u8, 1);
    }
    else
    {
        uint8_t u8 = ((uint8_t)bit_max) | flag;

        co_byte_array_add(buffer, &u8, 1);

        value -= bit_max;

        while (value >= 128)
        {
            u8 = (uint8_t)(value % 128 + 128);

            co_byte_array_add(buffer, &u8, 1);

            value /= 128;
        }

        u8 = (uint8_t)value;
        co_byte_array_add(buffer, &u8, 1);
    }
}

static bool
co_http2_hpack_deserialize_int(
    uint8_t bit_max,
    const uint8_t* data,
    size_t data_size,
    uint32_t* value,
    size_t* index
)
{
    if (data_size == 0)
    {
        return false;
    }

    (*value) = data[0] & ~(bit_max + 1);
    (*index) = sizeof(uint8_t);

    if ((*value) >= bit_max)
    {
        uint8_t b = 0;
        uint32_t m = 1;

        do
        {
            if (data_size <= (*index))
            {
                return false;
            }

            b = data[(*index)];

            (*index) += sizeof(uint8_t);

            (*value) += ((b & 127) * m);

            m <<= 7;

        } while ((b & 128) == 128);
    }

    return true;
}

#if 0
static void
co_http2_hpack_serialize_4bits_int(
    bool flag,
    uint32_t value,
    co_byte_array_t* buffer
)
{
    co_http2_hpack_serialize_int(
        CO_HTTP2_MAX_4_BITS,
        (flag ? CO_HTTP2_FLAG_4_BITS : 0),
        value, buffer);
}
#endif

#if 0
static void
co_http2_hpack_serialize_5bits_int(
    bool flag,
    uint32_t value,
    co_byte_array_t* buffer
)
{
    co_http2_hpack_serialize_int(
        CO_HTTP2_MAX_5_BITS,
        (flag ? CO_HTTP2_FLAG_5_BITS : 0),
        value, buffer);
}
#endif

static void
co_http2_hpack_serialize_6bits_int(
    bool flag,
    uint32_t value,
    co_byte_array_t* buffer
)
{
    co_http2_hpack_serialize_int(
        CO_HTTP2_MAX_6_BITS,
        (flag ? CO_HTTP2_FLAG_6_BITS : 0),
        value, buffer);
}

static void
co_http2_hpack_serialize_7bits_int(
    bool flag,
    uint32_t value,
    co_byte_array_t* buffer
)
{
    co_http2_hpack_serialize_int(
        CO_HTTP2_MAX_7_BITS,
        (flag ? CO_HTTP2_FLAG_7_BITS : 0),
        value, buffer);
}

static void
co_http2_hpack_serialize_string(
    bool encoding,
    const char* str,
    uint32_t str_length,
    co_byte_array_t* buffer
)
{
    if (encoding)
    {
        uint8_t* encoded_str = NULL;
        size_t encoded_str_length = 0;

        co_http2_huffman_encode(
            str, str_length,
            &encoded_str, &encoded_str_length);

        co_http2_hpack_serialize_7bits_int(
            encoding, (uint32_t)encoded_str_length, buffer);

        co_byte_array_add(
            buffer, encoded_str, encoded_str_length);

        co_mem_free(encoded_str);
    }
    else
    {
        co_http2_hpack_serialize_7bits_int(
            encoding, str_length, buffer);
        
        co_byte_array_add(
            buffer, str, str_length);
    }
}

static bool
co_http2_hpack_deserialize_string(
    const uint8_t* data,
    size_t data_size,
    char** str,
    uint32_t* str_length,
    size_t* index
)
{
    if (!co_http2_hpack_deserialize_int(
        CO_HTTP2_MAX_7_BITS, data, data_size, str_length, index))
    {
        return false;
    }

    if ((data_size - (*index)) < (size_t)(*str_length))
    {
        return false;
    }

    if ((*data) & CO_HTTP2_FLAG_7_BITS)
    {
        size_t dest_length = 0;

        if (!co_http2_huffman_decode(
            &data[(*index)], (*str_length),
            str, &dest_length))
        {
            return false;
        }

        (*index) += (*str_length);

        (*str_length) = (uint32_t)dest_length;
    }
    else
    {
        (*str) = co_string_duplicate_n(
            (const char*)&data[(*index)],
            (*str_length));

        (*index) += (*str_length);
    }

    if ((*str) == NULL)
    {
        return false;
    }

    return true;
}

static void
co_http2_hpack_dynamic_table_item_destroy(
    co_http2_hpack_dynamic_table_item_t* item
)
{
    co_string_destroy(item->name);
    co_string_destroy(item->value);
    co_mem_free(item);
}

void
co_http2_hpack_dynamic_table_setup(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t max_size
)
{
    dynamic_table->max_size = max_size;
    dynamic_table->total_size = 0;

    co_list_ctx_st ctx = { 0 };
    ctx.destroy_value =
        (co_item_destroy_fn)co_http2_hpack_dynamic_table_item_destroy;

    dynamic_table->items = co_list_create(&ctx);
}

void
co_http2_hpack_dynamic_table_cleanup(
    co_http2_hpack_dynamic_table_t* dynamic_table
)
{
    if (dynamic_table != NULL)
    {
        co_list_destroy(dynamic_table->items);
        dynamic_table->items = NULL;
    }
}

static void
co_http2_hpack_dynamic_table_resize(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t max_size
)
{
    dynamic_table->max_size = max_size;

    if (dynamic_table->max_size == 0)
    {
        dynamic_table->total_size = 0;

        co_list_clear(dynamic_table->items);

        return;
    }

    while (dynamic_table->total_size > dynamic_table->max_size)
    {
        co_list_data_st* data =
            co_list_get_tail(dynamic_table->items);
        co_http2_hpack_dynamic_table_item_t* old_item =
            (co_http2_hpack_dynamic_table_item_t*)data->value;

        dynamic_table->total_size -= old_item->size;

        co_list_remove_tail(dynamic_table->items);
    }
}

static bool
co_http2_hpack_dynamic_table_add_item(
    co_http2_hpack_dynamic_table_t* dynamic_table,
    const char* name,
    const char* value
)
{
    co_http2_hpack_dynamic_table_item_t* item =
        (co_http2_hpack_dynamic_table_item_t*)co_mem_alloc(
            sizeof(co_http2_hpack_dynamic_table_item_t));

    if (item == NULL)
    {
        return false;
    }

    size_t name_length = strlen(name);
    item->name = co_string_duplicate_n(name, name_length);

    if (item->name == NULL)
    {
        co_mem_free(item);

        return false;
    }

    size_t value_length = strlen(value);
    item->value = co_string_duplicate_n(value, value_length);

    if (item->value == NULL)
    {
        co_string_destroy(item->name);
        co_mem_free(item);

        return false;
    }

    item->size = (uint32_t)(name_length + value_length + 32);

    if (!co_list_add_head(dynamic_table->items, item))
    {
        co_string_destroy(item->name);
        co_string_destroy(item->value);
        co_mem_free(item);

        return false;
    }

    dynamic_table->total_size += item->size;

    while (dynamic_table->total_size > dynamic_table->max_size)
    {
        co_list_data_st* data =
            co_list_get_tail(dynamic_table->items);
        co_http2_hpack_dynamic_table_item_t* old_item =
            (co_http2_hpack_dynamic_table_item_t*)data->value;

        dynamic_table->total_size -= old_item->size;

        co_list_remove_tail(dynamic_table->items);
    }

    return true;
}

static bool
co_http2_hpack_dynamic_table_find_item(
    const co_http2_hpack_dynamic_table_t* dynamic_table,
    const char* name,
    const char* value,
    uint32_t* header_index
)
{
    (*header_index) = 62;

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(dynamic_table->items);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_const_next(dynamic_table->items, &it);
        const co_http2_hpack_dynamic_table_item_t* item =
            (const co_http2_hpack_dynamic_table_item_t*)data->value;

        if ((co_string_case_compare(item->name, name) == 0) &&
            (co_string_case_compare(item->value, value) == 0))
        {
            return true;
        }

        ++(*header_index);
    }

    return false;
}

static bool
co_http2_hpack_dynamic_table_find_item_by_index(
    const co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t header_index,
    char** name,
    char** value
)
{
    header_index -= 61;

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(dynamic_table->items);
    const co_list_data_st* data = NULL;

    while ((it != NULL) && (header_index != 0))
    {
        data = co_list_get_const_next(dynamic_table->items, &it);
        --header_index;
    }

    if ((data == NULL) || (header_index != 0))
    {
        return false;
    }

    const co_http2_hpack_dynamic_table_item_t* item =
        (const co_http2_hpack_dynamic_table_item_t*)data->value;

    if (name != NULL)
    {
        (*name) = co_string_duplicate(item->name);
    }

    if (value != NULL)
    {
        (*value) = co_string_duplicate(item->value);
    }

    return true;
}

static bool
co_http2_hpack_static_table_find_item(
    const char* name,
    uint32_t* header_index
)
{
    size_t hash = co_string_hash(name) % 285;
    uint32_t index = hpack_static_hash_name_table[hash];

    if ((index != 0xff) &&
        (co_string_case_compare(
            hpack_static_name_table[index], name) == 0))
    {
        (*header_index) = (index + 1);

        return true;
    }

    return false;
}

static void
co_http2_hpack_serialize_header_field(
    const char* name,
    const char* value,
    co_http2_hpack_dynamic_table_t* dynamic_table,
    co_byte_array_t* buffer
)
{
    uint32_t header_index = 0;

    if (co_http2_hpack_dynamic_table_find_item(
        dynamic_table, name, value, &header_index))
    {
        co_http2_hpack_serialize_7bits_int(true, header_index, buffer);
    }
    else if (co_http2_hpack_static_table_find_item(name, &header_index))
    {
        co_http2_hpack_serialize_6bits_int(true, header_index, buffer);
        co_http2_hpack_serialize_string(
            true, value, (uint32_t)strlen(value), buffer);
        co_http2_hpack_dynamic_table_add_item(dynamic_table, name, value);
    }
    else
    {
        co_http2_hpack_serialize_6bits_int(true, 0, buffer);
        co_http2_hpack_serialize_string(
            true, name, (uint32_t)strlen(name), buffer);
        co_http2_hpack_serialize_string(
            true, value, (uint32_t)strlen(value), buffer);
        co_http2_hpack_dynamic_table_add_item(dynamic_table, name, value);
    }
}

void
co_http2_hpack_serialize_header(
    const co_http2_header_t* header,
    co_http2_hpack_dynamic_table_t* dynamic_table,
    co_byte_array_t* buffer
)
{
    if (header->pseudo.method != NULL)
    {
        if (co_string_case_compare(header->pseudo.method, "GET") == 0)
        {
            co_http2_hpack_serialize_7bits_int(true, 2, buffer);
        }
        else if (co_string_case_compare(header->pseudo.method, "POST") == 0)
        {
            co_http2_hpack_serialize_7bits_int(true, 3, buffer);
        }
        else
        {
            co_http2_hpack_serialize_header_field(
                ":method", header->pseudo.method, dynamic_table, buffer);
        }
    }

    if (header->pseudo.protocol != NULL)
    {
        co_http2_hpack_serialize_header_field(
            ":protocol", header->pseudo.protocol, dynamic_table, buffer);
    }
    
    if (header->pseudo.scheme != NULL)
    {
        if (co_string_case_compare(header->pseudo.scheme, "http") == 0)
        {
            co_http2_hpack_serialize_7bits_int(true, 6, buffer);
        }
        else if (co_string_case_compare(header->pseudo.scheme, "https") == 0)
        {
            co_http2_hpack_serialize_7bits_int(true, 7, buffer);
        }
        else
        {
            co_http2_hpack_serialize_header_field(
                ":scheme", header->pseudo.scheme, dynamic_table, buffer);
        }
    }

    if (header->pseudo.url != NULL)
    {
        if (header->pseudo.url->query == NULL)
        {
            if (co_string_case_compare(header->pseudo.url->path, "/") == 0)
            {
                co_http2_hpack_serialize_7bits_int(true, 4, buffer);
            }
            else if (co_string_case_compare(header->pseudo.url->path, "/index.html") == 0)
            {
                co_http2_hpack_serialize_7bits_int(true, 5, buffer);
            }
            else
            {
                co_http2_hpack_serialize_header_field(
                    ":path", header->pseudo.url->path, dynamic_table, buffer);
            }
        }
        else
        {
            char* path_and_query =
                co_http_url_create_path_and_query(header->pseudo.url);

            co_http2_hpack_serialize_header_field(
                ":path", path_and_query, dynamic_table, buffer);

            co_string_destroy(path_and_query);
        }
    }

    if (header->pseudo.authority != NULL)
    {
        co_http2_hpack_serialize_header_field(
            ":authority", header->pseudo.authority, dynamic_table, buffer);
    }

    if (header->pseudo.status_code != 0)
    {
        switch (header->pseudo.status_code)
        {
        case 200:
        {
            co_http2_hpack_serialize_7bits_int(true, 8, buffer);

            break;
        }
        case 204:
        {
            co_http2_hpack_serialize_7bits_int(true, 9, buffer);

            break;
        }
        case 206:
        {
            co_http2_hpack_serialize_7bits_int(true, 10, buffer);

            break;
        }
        case 304:
        {
            co_http2_hpack_serialize_7bits_int(true, 11, buffer);

            break;
        }
        case 400:
        {
            co_http2_hpack_serialize_7bits_int(true, 12, buffer);

            break;
        }
        case 404:
        {
            co_http2_hpack_serialize_7bits_int(true, 13, buffer);

            break;
        }
        case 500:
        {
            co_http2_hpack_serialize_7bits_int(true, 14, buffer);

            break;
        }
        default:
        {
            char status_code_str[8];
            sprintf(status_code_str, "%hu", header->pseudo.status_code);

            co_http2_hpack_serialize_header_field(
                ":status", status_code_str, dynamic_table, buffer);

            break;
        }
        }
    }

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_const_next(header->field_list, &it);
        const co_http2_header_field_t* field =
            (const co_http2_header_field_t*)data->value;

        co_http2_hpack_serialize_header_field(
            field->name, field->value, dynamic_table, buffer);
    }
}

static bool
co_http2_hpack_deserialize_header_field(
    uint8_t bits,
    const uint8_t* data,
    size_t data_size,
    const co_http2_hpack_dynamic_table_t* dynamic_table,
    uint32_t* header_index,
    char** name,
    char** value,
    size_t* index
)
{
    size_t temp_index = 0;
    uint32_t str_length = 0;

    (*index) = 0;

    if (!co_http2_hpack_deserialize_int(
        bits, data, data_size, header_index, &temp_index))
    {
        return false;
    }

    (*index) += temp_index;
    data_size -= temp_index;
    data += temp_index;

    if ((*header_index) == 0)
    {
        if (!co_http2_hpack_deserialize_string(
            data, data_size, name, &str_length, &temp_index))
        {
            return false;
        }

        (*index) += temp_index;
        data_size -= temp_index;
        data += temp_index;

        if (!co_http2_hpack_deserialize_string(
            data, data_size, value, &str_length, &temp_index))
        {
            return false;
        }
    }
    else
    {
        if ((*header_index) <= 61)
        {
            (*name) = co_string_duplicate(
                hpack_static_name_table[(*header_index) - 1]);
        }
        else
        {            
            if (!co_http2_hpack_dynamic_table_find_item_by_index(
                dynamic_table, (*header_index), name, NULL))
            {
                return false;
            }
        }

        if (!co_http2_hpack_deserialize_string(
            data, data_size, value, &str_length, &temp_index))
        {
            return false;
        }
    }

    (*index) += temp_index;

    return true;
}

bool
co_http2_hpack_deserialize_header(
    const uint8_t* data,
    size_t data_size,
    co_http2_hpack_dynamic_table_t* dynamic_table,
    co_http2_header_t* header
)
{
    while (data_size != 0)
    {
        uint8_t prefix = *data;

        uint32_t header_index = 0;
        size_t index = 0;

        char* name = NULL;
        char* value = NULL;

        if (prefix & CO_HTTP2_FLAG_7_BITS)
        {
            if (!co_http2_hpack_deserialize_int(
                CO_HTTP2_MAX_7_BITS, data, data_size, &header_index, &index))
            {
                return false;
            }

            if (header_index == 0)
            {
                return false;
            }

            if (header_index <= 61)
            {
                name = co_string_duplicate(
                    hpack_static_name_table[header_index - 1]);
                value = co_string_duplicate(
                    hpack_static_value_table[header_index - 1]);
            }
            else
            {
                if (!co_http2_hpack_dynamic_table_find_item_by_index(
                    dynamic_table, header_index, &name, &value))
                {
                    return false;
                }
            }
        }
        else if (prefix & CO_HTTP2_FLAG_6_BITS)
        {
            if (!co_http2_hpack_deserialize_header_field(
                CO_HTTP2_MAX_6_BITS, data, data_size, dynamic_table,
                &header_index, &name, &value, &index))
            {
                return false;
            }

            co_http2_hpack_dynamic_table_add_item(
                dynamic_table, name, value);
        }
        else if (prefix & CO_HTTP2_FLAG_5_BITS)
        {
            uint32_t max_header_list_size = 0;

            if (!co_http2_hpack_deserialize_int(
                CO_HTTP2_MAX_5_BITS, data, data_size,
                &max_header_list_size, &index))
            {
                return false;
            }

            co_http2_hpack_dynamic_table_resize(
                dynamic_table, max_header_list_size);
        }
        else
        {
            if (!co_http2_hpack_deserialize_header_field(
                CO_HTTP2_MAX_4_BITS, data, data_size, dynamic_table,
                &header_index, &name, &value, &index))
            {
                return false;
            }
        }

        if ((name != NULL) && (value != NULL))
        {
            if (co_string_case_compare(name, ":authority") == 0)
            {
                header->pseudo.authority = value;

                co_string_destroy(name);
            }
            else if (co_string_case_compare(name, ":method") == 0)
            {
                header->pseudo.method = value;

                co_string_destroy(name);
            }
            else if (co_string_case_compare(name, ":path") == 0)
            {
                header->pseudo.url = co_http_url_create(value);

                co_string_destroy(name);
                co_string_destroy(value);

                if (header->pseudo.url == NULL)
                {
                    return false;
                }
            }
            else if (co_string_case_compare(name, ":scheme") == 0)
            {
                header->pseudo.scheme = value;

                co_string_destroy(name);
            }
            else if (co_string_case_compare(name, ":status") == 0)
            {
                header->pseudo.status_code = (uint16_t)strtoul(value, NULL, 10);

                co_string_destroy(name);
                co_string_destroy(value);
            }
            else if (co_string_case_compare(name, ":protocol") == 0)
            {
                header->pseudo.protocol = value;

                co_string_destroy(name);
            }
            else
            {
                co_http2_header_add_field_ptr(header, name, value);
            }
        }

        data_size -= index;
        data += index;
    }

    return true;
}
