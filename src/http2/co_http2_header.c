#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http2/co_http2_header.h>

//---------------------------------------------------------------------------//
// http2 header
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http2_header_field_destroy(
    co_http2_header_field_t* field
)
{
    co_string_destroy(field->name);
    co_string_destroy(field->value);
    co_mem_free(field);
}

intptr_t
co_http2_header_field_compare(
    const co_http2_header_field_t* field1,
    const co_http2_header_field_t* field2
)
{
    return co_string_case_compare(field1->name, field2->name);
}

bool
co_http2_header_add_field_ptr(
    co_http2_header_t* header,
    char* name,
    char* value
)
{
    co_http2_header_field_t* field =
        (co_http2_header_field_t*)co_mem_alloc(
            sizeof(co_http2_header_field_t));

    if (field == NULL)
    {
        return false;
    }

    field->name = name;
    field->value = value;

    return co_list_add_tail(header->field_list, (uintptr_t)field);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http2_header_t*
co_http2_header_create(
    void
)
{
    co_http2_header_t* header =
        (co_http2_header_t*)co_mem_alloc(sizeof(co_http2_header_t));

    if (header == NULL)
    {
        return NULL;
    }

    memset(&header->pseudo, 0x00, sizeof(co_http2_pseudo_header_t));

    co_list_ctx_st list_ctx = { 0 };
#if (__GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
    list_ctx.destroy_value =
        (co_item_destroy_fn)co_http2_header_field_destroy;
    list_ctx.compare_values =
        (co_item_compare_fn)co_http2_header_field_compare;
#if (__GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif
    header->field_list = co_list_create(&list_ctx);

    header->stream_dependency = 0;
    header->weight = 0;

    return header;
}

co_http2_header_t*
co_http2_header_create_request(
    const char* method,
    const char* path
)
{
    co_http2_header_t* header = co_http2_header_create();

    if (header == NULL)
    {
        return NULL;
    }

    co_http2_header_set_method(header, method);
    co_http2_header_set_path(header, path);

    return header;
}

co_http2_header_t*
co_http2_header_create_response(
    uint16_t status_code
)
{
    co_http2_header_t* header = co_http2_header_create();

    if (header == NULL)
    {
        return NULL;
    }

    co_http2_header_set_status_code(header, status_code);

    return header;
}

void
co_http2_header_destroy(
    co_http2_header_t* header
)
{
    if (header != NULL)
    {
        co_string_destroy(header->pseudo.authority);
        co_string_destroy(header->pseudo.method);
        co_string_destroy(header->pseudo.scheme);
        co_http_url_destroy(header->pseudo.url);

        co_list_destroy(header->field_list);
        header->field_list = NULL;

        co_mem_free(header);
    }
}

void
co_http2_header_clear(
    co_http2_header_t* header
)
{
    if (header != NULL)
    {
        co_list_clear(header->field_list);
    }
}

void
co_http2_header_set_authority(
    co_http2_header_t* header,
    const char* authority
)
{
    co_string_destroy(header->pseudo.authority);

    if (authority != NULL)
    {
        header->pseudo.authority = co_string_duplicate(authority);
    }
    else
    {
        header->pseudo.authority = NULL;
    }
}

const char*
co_http2_header_get_authority(
    const co_http2_header_t* header
)
{
    return header->pseudo.authority;
}

void
co_http2_header_set_method(
    co_http2_header_t* header,
    const char* method
)
{
    co_string_destroy(header->pseudo.method);

    if (method != NULL)
    {
        header->pseudo.method = co_string_duplicate(method);
    }
    else
    {
        header->pseudo.method = NULL;
    }
}

const char*
co_http2_header_get_method(
    const co_http2_header_t* header
)
{
    return header->pseudo.method;
}

void
co_http2_header_set_path(
    co_http2_header_t* header,
    const char* path
)
{
    co_http_url_destroy(header->pseudo.url);

    if (path != NULL)
    {
        header->pseudo.url = co_http_url_create(path);

        if (header->pseudo.url->path == NULL)
        {
            header->pseudo.url->path = co_string_duplicate("/");
        }
    }
    else
    {
        header->pseudo.url = NULL;
    }
}

const char*
co_http2_header_get_path(
    const co_http2_header_t* header
)
{
    if (header->pseudo.url == NULL)
    {
        return NULL;
    }

    return header->pseudo.url->src;
}

const co_http_url_st*
co_http2_header_get_path_url(
    const co_http2_header_t* header
)
{
    return header->pseudo.url;
}

void
co_http2_header_set_scheme(
    co_http2_header_t* header,
    const char* scheme
)
{
    co_string_destroy(header->pseudo.scheme);

    if (scheme != NULL)
    {
        header->pseudo.scheme = co_string_duplicate(scheme);
    }
    else
    {
        header->pseudo.scheme = NULL;
    }
}

const char*
co_http2_header_get_scheme(
    const co_http2_header_t* header
)
{
    return header->pseudo.scheme;
}

void
co_http2_header_set_status_code(
    co_http2_header_t* header,
    uint16_t status_code
)
{
    header->pseudo.status_code = status_code;
}

uint16_t
co_http2_header_get_status_code(
    const co_http2_header_t* header
)
{
    return header->pseudo.status_code;
}

void
co_http2_header_set_stream_dependency(
    co_http2_header_t* header,
    uint32_t stream_dependency
)
{
    header->stream_dependency = stream_dependency;
}

uint32_t
co_http2_header_get_stream_dependency(
    const co_http2_header_t* header
)
{
    return header->stream_dependency;
}

void
co_http2_header_set_weight(
    co_http2_header_t* header,
    uint8_t weight
)
{
    header->weight = weight;
}

uint8_t
co_http2_header_get_weight(
    const co_http2_header_t* header
)
{
    return header->weight;
}

size_t
co_http2_header_get_field_count(
    const co_http2_header_t* header
)
{
    return co_list_get_count(header->field_list);
}

size_t
co_http2_header_get_value_count(
    const co_http2_header_t* header,
    const char* name
)
{
    size_t value_count = 0;

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_const_next(header->field_list, &it);

        if (co_string_case_compare(
            ((const co_http2_header_field_t*)data->value)->name, name) == 0)
        {
            ++value_count;
        }
    }

    return value_count;
}

bool
co_http2_header_contains(
    const co_http2_header_t* header,
    const char* name
)
{
    const co_http2_header_field_t find_field = { (char*)name, NULL };

    const co_list_iterator_t* it =
        co_list_find_const(header->field_list, (uintptr_t)&find_field);

    return (it != NULL);
}

void
co_http2_header_set_field(
    co_http2_header_t* header,
    const char* name,
    const char* value
)
{
    const co_http2_header_field_t find_field = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->field_list, (uintptr_t)&find_field);

    if (it != NULL)
    {
        co_http2_header_field_t* field =
            (co_http2_header_field_t*)it->data.value;

        co_string_destroy(field->value);

        field->value = co_string_duplicate(value);
    }
    else
    {
        co_http2_header_add_field(header, name, value);
    }
}

const char*
co_http2_header_get_field(
    const co_http2_header_t* header,
    const char* name
)
{
    const co_http2_header_field_t find_field = { (char*)name, NULL };

    const co_list_iterator_t* it =
        co_list_find_const(header->field_list, (uintptr_t)&find_field);

    if (it != NULL)
    {
        const co_http2_header_field_t* field =
            (const co_http2_header_field_t*)co_list_get_const(
                header->field_list, it)->value;

        return field->value;
    }

    return NULL;
}

size_t
co_http2_header_get_fields(
    const co_http2_header_t* header,
    const char* name,
    const char* value[],
    size_t count
)
{
    size_t value_count = 0;

    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(header->field_list);

    while (it != NULL && (value_count < count))
    {
        const co_list_data_st* data =
            co_list_get_const_next(header->field_list, &it);

        if (co_string_case_compare(
            ((const co_http2_header_field_t*)data->value)->name, name) == 0)
        {
            value[value_count] =
                ((const co_http2_header_field_t*)data->value)->value;

            ++value_count;
        }
    }

    return value_count;
}

bool
co_http2_header_add_field(
    co_http2_header_t* header,
    const char* name,
    const char* value
)
{
    co_http2_header_field_t* field =
        (co_http2_header_field_t*)co_mem_alloc(
            sizeof(co_http2_header_field_t));

    if (field == NULL)
    {
        return false;
    }

    field->name = co_string_duplicate(name);

    if (field->name == NULL)
    {
        co_mem_free(field);

        return false;
    }

    field->value = co_string_duplicate(value);

    if (field->value == NULL)
    {
        co_string_destroy(field->name);
        co_mem_free(field);

        return false;
    }

    return co_list_add_tail(header->field_list, (uintptr_t)field);
}

void
co_http2_header_remove_field(
    co_http2_header_t* header,
    const char* name
)
{
    co_http2_header_field_t find_field = { (char*)name, NULL };

    co_list_remove(header->field_list, (uintptr_t)&find_field);
}

void
co_http2_header_remove_all_fields(
    co_http2_header_t* header,
    const char* name
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data = co_list_get(header->field_list, it);

        if (co_string_case_compare(
            ((co_http2_header_field_t*)data->value)->name, name) == 0)
        {
            co_list_iterator_t* temp = it;
            it = co_list_get_next_iterator(header->field_list, it);

            co_list_remove_at(header->field_list, temp);
        }
        else
        {
            it = co_list_get_next_iterator(header->field_list, it);
        }
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http2_header_add_client_cookie(
    co_http2_header_t* header,
    const co_http_cookie_st* cookie
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    co_http_request_cookie_serialize(cookie, 1, buffer);

    co_http2_header_add_field_ptr(header,
        co_string_duplicate(CO_HTTP2_HEADER_COOKIE),
        (char*)co_byte_array_detach(buffer));

    co_byte_array_destroy(buffer);
}

size_t
co_http2_header_get_client_cookies(
    const co_http2_header_t* header,
    co_http_cookie_st* cookies,
    size_t count
)
{
    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(header->field_list);

    size_t index = 0;

    while ((it != NULL) && (index < count))
    {
        const co_list_data_st* data =
            co_list_get_const_next(header->field_list, &it);

        const co_http2_header_field_t* field =
            (const co_http2_header_field_t*)data->value;

        if (co_string_case_compare(
            field->name, CO_HTTP2_HEADER_COOKIE) == 0)
        {
            size_t result = co_http_request_cookie_deserialize(
                field->value, cookies, count - index);

            if (result == 0)
            {
                break;
            }

            index += result;
        }
    }

    return index;
}

void
co_http2_header_remove_all_client_cookies(
    co_http2_header_t* header
)
{
    co_http2_header_remove_all_fields(
        header, CO_HTTP2_HEADER_COOKIE);
}

void
co_http2_header_add_server_cookie(
    co_http2_header_t* header,
    const co_http_cookie_st* cookie
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    co_http_response_cookie_serialize(cookie, buffer);

    co_http2_header_add_field_ptr(header,
        co_string_duplicate(CO_HTTP2_HEADER_SET_COOKIE),
        (char*)co_byte_array_detach(buffer));

    co_byte_array_destroy(buffer);
}

size_t
co_http2_header_get_server_cookies(
    const co_http2_header_t* header,
    co_http_cookie_st* cookies,
    size_t count
)
{
    const co_list_iterator_t* it =
        co_list_get_const_head_iterator(header->field_list);

    size_t index = 0;

    while ((it != NULL) && (index < count))
    {
        const co_list_data_st* data =
            co_list_get_const_next(header->field_list, &it);

        const co_http2_header_field_t* field =
            (const co_http2_header_field_t*)data->value;

        if (co_string_case_compare(
            field->name, CO_HTTP2_HEADER_SET_COOKIE) == 0)
        {
            if (co_http_response_cookie_deserialize(
                field->value, &cookies[index]))
            {
                ++index;
            }
        }
    }

    return index;
}

void
co_http2_header_remove_all_server_cookies(
    co_http2_header_t* header
)
{
    co_http2_header_remove_all_fields(
        header, CO_HTTP2_HEADER_SET_COOKIE);
}
