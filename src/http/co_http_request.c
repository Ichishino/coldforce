#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_request.h>
#include <coldforce/http/co_http_config.h>

//---------------------------------------------------------------------------//
// http request
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http_request_serialize(
    const co_http_request_t* request,
    co_byte_array_t* buffer
)
{
    co_byte_array_add_string(buffer, request->method);
    co_byte_array_add_string(buffer, CO_HTTP_SP);
    co_byte_array_add_string(buffer, request->url->src);
    
    co_byte_array_add_string(buffer, CO_HTTP_SP);
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
        (const char*)co_byte_array_get_const_ptr(data, *index);
    const size_t data_size = co_byte_array_get_count(data) - (*index);

    const char* new_line =
        co_string_find_n(data_ptr, CO_HTTP_CRLF, data_size);

    const size_t max_header_line_size =
        co_http_config_get_max_receive_header_line_size();

    if (new_line == NULL)
    {
        if (data_size > max_header_line_size)
        {
            return CO_HTTP_ERROR_HEADER_LINE_TOO_LONG;
        }
        else
        {
            return CO_HTTP_PARSE_MORE_DATA;
        }
    }

    size_t length = (new_line - data_ptr);

    if (length > max_header_line_size)
    {
        return CO_HTTP_ERROR_HEADER_LINE_TOO_LONG;
    }

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

    if ((strcmp(method, "GET") != 0) &&
        (strcmp(method, "POST") != 0) &&
        (strcmp(method, "PUT") != 0) &&
        (strcmp(method, "DELETE") != 0) &&
        (strcmp(method, "PATCH") != 0) &&
        (strcmp(method, "HEAD") != 0) &&
        (strcmp(method, "OPTIONS") != 0) &&
        (strcmp(method, "CONNECT") != 0) &&
        (strcmp(method, "TRACE") != 0))
    {
        if ((data_size >= 24) &&
            (memcmp(&data_ptr[temp_index],
                "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) == 0))
        {
            request->method = method;
            request->url = co_http_url_create("*");
            request->version = co_string_duplicate("HTTP/2.0");

            (*index) += 24;

            return CO_HTTP_PARSE_COMPLETE;
        }

        co_string_destroy(method);

        return CO_HTTP_PARSE_ERROR;
    }

    length -= (item_length + 1);
    temp_index += (item_length + 1);

    sp = co_string_find_n(&data_ptr[temp_index], " ", length);

    if (sp == NULL)
    {
        co_string_destroy(method);

        return CO_HTTP_PARSE_ERROR;
    }

    item_length = sp - &data_ptr[temp_index];

    char* url_str = co_string_duplicate_n(&data_ptr[temp_index], item_length);

    co_http_url_st* url = co_http_url_create(url_str);

    if (url->path == NULL)
    {
        co_string_destroy(method);
        co_http_url_destroy(url);

        return CO_HTTP_PARSE_ERROR;
    }

    length -= (item_length + 1);
    temp_index += (item_length + 1);

    item_length = length;

    char* version = co_string_duplicate_n(&data_ptr[temp_index], item_length);

    request->method = method;
    request->url = url;
    request->version = version;

    co_string_destroy(url_str);

    temp_index += (item_length + CO_HTTP_CRLF_LENGTH) + (*index);

    int result =
        co_http_message_deserialize_header(
            &request->message, data, &temp_index);

    if (result == CO_HTTP_PARSE_COMPLETE)
    {
        (*index) = temp_index;
    }

    return result;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http_request_t*
co_http_request_create(
    void
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

    return request;
}

co_http_request_t*
co_http_request_create_with(
    const char* method,
    const char* path
)
{
    co_http_request_t* request = co_http_request_create();

    if (request == NULL)
    {
        return NULL;
    }

    co_http_request_set_method(request, method);    
    co_http_request_set_path(request, path);
    co_http_request_set_version(request, CO_HTTP_VERSION_1_1);

    return request;
}

void
co_http_request_destroy(
    co_http_request_t* request
)
{
    if (request != NULL)
    {
        co_string_destroy(request->method);
        co_string_destroy(request->version);

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
co_http_request_set_data(
    co_http_request_t* request,
    const void* data,
    size_t data_size
)
{
    return co_http_message_set_data(
        &request->message, data, data_size);
}

const void*
co_http_request_get_data(
    const co_http_request_t* request
)
{
    return co_http_message_get_data(
        &request->message);
}

size_t
co_http_request_get_data_size(
    const co_http_request_t* request
)
{
    return co_http_message_get_data_size(
        &request->message);
}

void
co_http_request_set_path(
    co_http_request_t* request,
    const char* path)
{
    co_http_url_destroy(request->url);

    if (path != NULL)
    {
        request->url = co_http_url_create(path);

        if (request->url->path == NULL)
        {
            request->url->path = co_string_duplicate("/");
        }
    }
    else
    {
        request->url = NULL;
    }
}

const char*
co_http_request_get_path(
    const co_http_request_t* request
)
{
    if (request->url == NULL)
    {
        return NULL;
    }

    return request->url->src;
}

const co_http_url_st*
co_http_request_get_url(
    const co_http_request_t* request
)
{
    return request->url;
}

void
co_http_request_set_method(
    co_http_request_t* request,
    const char* method
)
{
    co_string_destroy(request->method);

    if (method != NULL)
    {
        request->method = co_string_duplicate(method);
    }
    else
    {
        request->method = NULL;
    }
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
    co_string_destroy(request->version);

    if (version != NULL)
    {
        request->version = co_string_duplicate(version);
    }
    else
    {
        request->version = NULL;
    }
}

const char*
co_http_request_get_version(
    const co_http_request_t* request
)
{
    return request->version;
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

    co_http_header_set_field(
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
    const char* value = co_http_header_get_field(
        &request->message.header, CO_HTTP_HEADER_COOKIE);

    if (value != NULL)
    {
        co_byte_array_t* buffer = co_byte_array_create();

        co_byte_array_add_string(buffer, value);

        co_http_request_cookie_serialize(cookie, 1, buffer);

        co_byte_array_add(buffer, "\0", 1);
        char* str = (char*)co_byte_array_detach(buffer);

        co_http_header_set_field(
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
    co_http_cookie_st* cookies,
    size_t count
)
{
    const co_http_header_t* header = &request->message.header;

    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);

    size_t index = 0;

    while ((it != NULL) && (index < count))
    {
        const co_list_data_st* data =
            co_list_get_next(header->field_list, &it);

        const co_http_header_field_t* field =
            (const co_http_header_field_t*)data->value;

        if (co_string_case_compare(
            field->name, CO_HTTP_HEADER_COOKIE) == 0)
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
co_http_request_remove_all_cookies(
    co_http_request_t* request
)
{
    co_http_header_remove_all_fields(
        &request->message.header, CO_HTTP_HEADER_COOKIE);
}

bool
co_http_request_apply_auth(
    co_http_request_t* request,
    const char* header_name,
    co_http_auth_t* auth
)
{
    if (co_string_case_compare(auth->scheme, "Digest") == 0)
    {
        co_http_digest_auth_set_method(auth, request->method);
        co_http_digest_auth_set_path(auth, request->url->src);
        co_http_digest_auth_set_count(auth, ++auth->nc);
    }

    char* auth_str =
        co_http_auth_serialize_request(auth);

    if (auth_str == NULL)
    {
        return false;
    }

    co_http_header_remove_all_fields(
        &request->message.header,
        header_name);

    co_http_header_add_field_ptr(
        &request->message.header,
        co_string_duplicate(header_name),
        auth_str);

    return true;
}

