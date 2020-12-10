#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_request.h>

//---------------------------------------------------------------------------//
// http request
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_request_serialize(
    const co_http_request_t* request,
    co_byte_array_t* buffer
)
{
    co_byte_array_add_string(buffer, request->method);
    co_byte_array_add_string(buffer, CO_HTTP_SP);
    co_byte_array_add_string(buffer, request->url->path);

    if (request->url->query != NULL)
    {
        co_byte_array_add_string(buffer, request->url->query);
    }
    
    co_byte_array_add_string(buffer, CO_HTTP_SP);
    co_byte_array_add_string(buffer, "HTTP/");
    co_byte_array_add_string(buffer, request->version);
    co_byte_array_add_string(buffer, CO_HTTP_CRLF);

    co_http_message_serialize(&request->message, buffer);
}

int
co_http_request_deserialize(
    co_http_request_t* request,
    const co_byte_array_t* data,
    size_t* index
)
{
    const char* data_ptr =
        (const char*)co_byte_array_get_ptr((co_byte_array_t*)data, *index);
    const size_t data_size = co_byte_array_get_count(data) - (*index);

    const char* new_line =
        co_string_find_n(data_ptr, CO_HTTP_CRLF, data_size);

    if (new_line == NULL)
    {
        return CO_HTTP_PARSE_MORE_DATA;
    }

    size_t length = (new_line - data_ptr);
    size_t item_length = 0;
    size_t temp_index = 0;

    const char* sp =
        co_string_find_n(&data_ptr[temp_index], " ", length);

    if (sp == NULL)
    {
        return CO_HTTP_PARSE_ERROR;
    }

    item_length = sp - &data_ptr[temp_index];

    char* method =
        co_string_duplicate_n(&data_ptr[temp_index], item_length);

    length -= (item_length + 1);
    temp_index += (item_length + 1);

    sp = co_string_find_n(&data_ptr[temp_index], " HTTP/", length);

    if (sp == NULL)
    {
        co_mem_free(method);

        return CO_HTTP_PARSE_ERROR;
    }

    item_length = sp - &data_ptr[temp_index];

    char* url = co_string_duplicate_n(&data_ptr[temp_index], item_length);

    length -= (item_length + 6);
    temp_index += (item_length + 6);

    item_length = length;

    char* version = co_string_duplicate_n(&data_ptr[temp_index], item_length);

    request->method = method;
    request->url = co_http_url_create(url);
    request->version = version;

    co_mem_free(url);

    (*index) += (new_line - data_ptr) + CO_HTTP_CRLF_LENGTH;

    return co_http_message_deserialize_header(data, index, &request->message);
}

void
co_http_request_print_header(
    const co_http_request_t* request
)
{
    if (request != NULL)
    {
        printf("--------\n");

        printf("%s %s HTTP/%s\n",
            co_http_request_get_method(request),
            co_http_request_get_url(request),
            co_http_request_get_version(request));

        co_http_header_print(&request->message.header);

        printf("--------\n");
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http_request_t*
co_http_request_create(
    const char* method,
    const char* url_str
)
{
    co_http_request_t* request =
        (co_http_request_t*)co_mem_alloc(sizeof(co_http_request_t));

    if (request == NULL)
    {
        return NULL;
    }

    co_http_message_setup(&request->message);

    request->url = NULL;
    request->method = NULL;
    request->version = NULL;
    request->save_file_path = NULL;

    co_http_request_set_method(request, method);
    co_http_request_set_url(request, url_str);

    return request;
}

void
co_http_request_destroy(
    co_http_request_t* request
)
{
    if (request != NULL)
    {
        co_mem_free(request->method);
        co_mem_free(request->version);
        co_mem_free(request->save_file_path);

        co_http_url_destroy(request->url);
        co_http_message_cleanup(&request->message);

        co_mem_free(request);
    }
}

co_http_header_t*
co_http_request_get_header(
    co_http_request_t* request
)
{
    return &request->message.header;
}

const co_http_header_t*
co_http_request_get_const_header(
    const co_http_request_t* request
)
{
    return &request->message.header;
}

bool
co_http_request_set_content(
    co_http_request_t* request,
    const void* data,
    size_t data_size
)
{
    return co_http_message_set_content(
        &request->message, data, data_size);
}

const void*
co_http_request_get_content(
    const co_http_request_t* request
)
{
    return co_http_message_get_content(
        &request->message);
}

size_t
co_http_request_get_content_size(
    const co_http_request_t* request
)
{
    return co_http_message_get_content_size(
        &request->message);
}

void
co_http_request_set_url(
    co_http_request_t* request,
    const char* url_str)
{
    co_http_url_destroy(request->url);

    request->url = co_http_url_create(url_str);

    if (request->url->path == NULL)
    {
        request->url->path = co_string_duplicate("/");
    }
}

const char*
co_http_request_get_url(
    const co_http_request_t* request
)
{
    return ((request->url != NULL) ? request->url->src : NULL);
}

void
co_http_request_set_method(
    co_http_request_t* request,
    const char* method
)
{
    co_mem_free(request->method);

    request->method = co_string_duplicate(method);
}

const char*
co_http_request_get_method(
    const co_http_request_t* request
)
{
    return request->method;
}

void
co_http_request_set_version(
    co_http_request_t* request,
    const char* version
)
{
    co_mem_free(request->version);

    request->version = co_string_duplicate(version);
}

const char*
co_http_request_get_version(
    const co_http_request_t* request
)
{
    return request->version;
}

void
co_http_request_set_save_file_path(
    co_http_request_t* request,
    const char* save_file_path
)
{
    co_mem_free(request->save_file_path);

    request->save_file_path = co_string_duplicate(save_file_path);
}

const char*
co_http_request_get_save_file_path(
    const co_http_request_t* request
)
{
    return request->save_file_path;
}

void
co_http_request_set_cookies(
    co_http_request_t* request,
    const co_http_cookie_st* cookie,
    size_t count)
{
    co_byte_array_t* buffer = co_byte_array_create();

    co_http_request_cookie_serialize(cookie, count, buffer);

    co_byte_array_add(buffer, "\0", 1);
    char* str = (char*)co_byte_array_detach(buffer);

    co_http_header_set_item(
        &request->message.header, CO_HTTP_HEADER_COOKIE, str);

    co_mem_free(str);
    co_byte_array_destroy(buffer);
}

void
co_http_request_add_cookie(
    co_http_request_t* request,
    const co_http_cookie_st* cookie
)
{
    const char* value = co_http_header_get_item(
        &request->message.header, CO_HTTP_HEADER_COOKIE);

    if (value != NULL)
    {
        co_byte_array_t* buffer = co_byte_array_create();

        co_byte_array_add_string(buffer, value);

        co_http_request_cookie_serialize(cookie, 1, buffer);

        co_byte_array_add(buffer, "\0", 1);
        char* str = (char*)co_byte_array_detach(buffer);

        co_http_header_set_item(
            &request->message.header, CO_HTTP_HEADER_COOKIE, str);

        co_mem_free(str);
        co_byte_array_destroy(buffer);
    }
    else
    {
        co_http_request_set_cookies(request, cookie, 1);
    }
}

size_t
co_http_request_get_cookies(
    const co_http_request_t* request,
    co_http_cookie_st* cookie,
    size_t count
)
{
    const co_http_header_t* header = &request->message.header;

    co_list_iterator_t* it =
        co_list_get_head_iterator(header->items);

    size_t index = 0;

    while ((it != NULL) && (index < count))
    {
        const co_list_data_st* data =
            co_list_get_next(header->items, &it);

        const co_http_header_item_t* item =
            (const co_http_header_item_t*)data->value;

        if (co_string_case_compare(
            item->name, CO_HTTP_HEADER_COOKIE) == 0)
        {
            size_t result = co_http_request_cookie_deserialize(
                item->value, cookie, count - index);

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
co_http_request_remove_all_cookies(
    co_http_request_t* request
)
{
    co_http_header_remove_all_values(
        &request->message.header, CO_HTTP_HEADER_COOKIE);
}
