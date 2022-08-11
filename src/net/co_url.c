#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>
#include <coldforce/core/co_string_token.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_net_addr_resolve.h>
#include <coldforce/net/co_url.h>

#include <ctype.h>

#ifndef CO_OS_WIN
#include <netdb.h>
#endif

//---------------------------------------------------------------------------//
// url
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static void*
co_url_query_duplicate_map_item(
    const void* key_or_value
)
{
    return (void*)key_or_value;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_url_st*
co_url_create(
    const char* str
)
{
    if (str == NULL)
    {
        return NULL;
    }

    co_url_st* url =
        (co_url_st*)co_mem_alloc(sizeof(co_url_st));

    if (url == NULL)
    {
        return NULL;
    }

    url->src = co_string_duplicate(str);

    url->scheme = NULL;
    url->user = NULL;
    url->password = NULL;
    url->host = NULL;
    url->port = 0;
    url->path = NULL;
    url->query = NULL;
    url->fragment = NULL;

    size_t length = strlen(str);
    char* url_str = co_string_duplicate_n(str, length);

    char* ptr = NULL;

    ptr = strrchr(url_str, '#');

    if (ptr != NULL)
    {
        url->fragment = co_string_duplicate_n(
            &url_str[ptr - url_str + 1], length - (ptr - url_str) - 1);

        url_str[ptr - url_str] = '\0';
    }

    ptr = strrchr(url_str, '?');

    if (ptr != NULL)
    {
        url->query = co_string_duplicate_n(
            &url_str[ptr - url_str + 1], length - (ptr - url_str) - 1);

        url_str[ptr - url_str] = '\0';
    }

    char* head = url_str;

    ptr = strstr(head, "://");

    if (ptr != NULL)
    {
        url->scheme = co_string_duplicate_n(head, ptr - head);

        head = ptr + 3;
    }

    ptr = strchr(head, '/');

    if (ptr != NULL)
    {
        url->path = co_string_duplicate(ptr);

        head[ptr - head] = '\0';
    }

    ptr = strchr(head, '@');

    if (ptr != NULL)
    {
        char* colon = strchr(head, ':');

        if (colon != NULL)
        {
            url->user =
                co_string_duplicate_n(head, colon - head);

            head = colon + 1;
        }

        url->password =
            co_string_duplicate_n(head, ptr - head);

        head = ptr + 1;
    }

    ptr = strchr(head, ':');

    if (ptr != NULL)
    {
        if (strlen(ptr + 1) <= 5)
        {
            url->port = (uint16_t)strtol(ptr + 1, NULL, 10);
        }

        url->host = co_string_duplicate_n(head, ptr - head);
    }
    else
    {
        if (strcmp(head, "*") == 0)
        {
            url->path = co_string_duplicate(head);
        }
        else
        {
            url->host = co_string_duplicate(head);
        }
    }

    co_string_destroy(url_str);

    return url;
}

void
co_url_destroy(
    co_url_st* url
)
{
    if (url != NULL)
    {
        co_string_destroy(url->src);
        co_string_destroy(url->scheme);
        co_string_destroy(url->user);
        co_string_destroy(url->password);
        co_string_destroy(url->host);
        co_string_destroy(url->path);
        co_string_destroy(url->query);
        co_string_destroy(url->fragment);

        co_mem_free(url);
    }
}

char*
co_url_create_base_url(
    const co_url_st* url
)
{
    if (url->scheme == NULL || url->host == NULL)
    {
        return NULL;
    }

    size_t length = strlen(url->scheme) + 3 + strlen(url->host);

    char* base_url = (char*)co_mem_alloc(length + 32);

    if (url->port != 0)
    {
        sprintf(base_url, "%s://%s:%u", url->scheme, url->host, url->port);
    }
    else
    {
        sprintf(base_url, "%s://%s", url->scheme, url->host);
    }

    return base_url;
}

char*
co_url_create_host_and_port(
    const co_url_st* url
)
{
    if (url->host == NULL)
    {
        return NULL;
    }
    else if (url->port == 0)
    {
        return co_string_duplicate(url->host);
    }

    size_t length = strlen(url->host);

    char* host_and_port = (char*)co_mem_alloc(length + 32);

    sprintf(host_and_port, "%s:%u", url->host, url->port);

    return host_and_port;
}

char*
co_url_create_path_and_query(
    const co_url_st* url
)
{
    if (url->path == NULL)
    {
        return co_string_duplicate("/");
    }
    else if (url->query == NULL)
    {
        return co_string_duplicate(url->path);
    }

    size_t length = strlen(url->path) + strlen(url->query);

    char* path_and_query = (char*)co_mem_alloc(length + 2);

    sprintf(path_and_query, "%s?%s", url->path, url->query);

    return path_and_query;
}

char*
co_url_create_file_name(
    const co_url_st* url
)
{
    if (url->path == NULL)
    {
        return NULL;
    }

    const char* str = strrchr(url->path, '/');

    if (str != NULL)
    {
        ++str;
    }
    else
    {
        str = url->path;
        ++str;
    }

    return co_string_duplicate(str);
}

bool
co_url_component_encode(
    const char* src,
    size_t src_length,
    char** dest,
    size_t* dest_length
)
{
    (*dest) = co_mem_alloc(src_length * 3 + 1);

    if ((*dest) == NULL)
    {
        return false;
    }

    (*dest_length) = 0;

    for (size_t index = 0; index < src_length; ++index)
    {
        if (isalnum((unsigned char)src[index]) ||
            (src[index] == '-') || (src[index] == '.') ||
            (src[index] == '_') || (src[index] == '~'))
        {
            (*dest)[(*dest_length)++] = src[index];
        }
/*
        else if (src[index] == ' ')
        {
            (*dest)[(*dest_length)++] = '+';
        }
*/
        else
        {
            char hex[3];
            sprintf(hex, "%02X", (unsigned char)src[index]);

            (*dest)[(*dest_length)++] = '%';
            (*dest)[(*dest_length)++] = hex[0];
            (*dest)[(*dest_length)++] = hex[1];
        }
    }

    (*dest)[(*dest_length)] = '\0';

    return true;
}

bool
co_url_component_decode(
    const char* src,
    size_t src_length,
    char** dest,
    size_t* dest_length
)
{
    *dest = co_mem_alloc(src_length + 1);

    if ((*dest) == NULL)
    {
        return false;
    }

    (*dest_length) = 0;

    for (size_t index = 0; index < src_length; ++index)
    {
        if (src[index] == '%')
        {
            if (((index + 2) >= src_length) ||
                (!isxdigit((unsigned char)src[index + 1]) ||
                    !isxdigit((unsigned char)src[index + 2])))
            {
                return false;
            }

            char hex[3] = { src[index + 1], src[index + 2], '\0' };

            (*dest)[(*dest_length)++] = (char)strtol(hex, NULL, 16);

            index += 2;
        }
        else if (src[index] == '+')
        {
            (*dest)[(*dest_length)++] = ' ';
        }
        else
        {
            (*dest)[(*dest_length)++] = src[index];
        }
    }

    (*dest)[(*dest_length)] = '\0';

    return true;
}

co_string_map_t*
co_url_query_parse(
    const char* src,
    bool unescape
)
{
    co_string_map_t* query_map = co_string_map_create();

    if (src != NULL)
    {
        co_string_token_st tokens[256];

        size_t count =
            co_string_token_split(src, tokens, 256);

        co_item_duplicate_fn duplicate_key =
            query_map->duplicate_key;
        co_item_duplicate_fn duplicate_value =
            query_map->duplicate_value;

        query_map->duplicate_key =
            co_url_query_duplicate_map_item;
        query_map->duplicate_value =
            co_url_query_duplicate_map_item;

        for (size_t index = 0; index < count; ++index)
        {
            char* key;
            char* value;

            if (unescape)
            {
                size_t key_length;

                co_url_component_decode(
                    tokens[index].first, strlen(tokens[index].first),
                    &key, &key_length);

                size_t value_length;

                co_url_component_decode(
                    tokens[index].second, strlen(tokens[index].second),
                    &value, &value_length);
            }
            else
            {
                key = tokens[index].first;
                tokens[index].first = NULL;

                value = tokens[index].second;
                tokens[index].second = NULL;
            }

            co_string_map_set(query_map, key, value);
        }

        query_map->duplicate_key =
            duplicate_key;
        query_map->duplicate_value =
            duplicate_value;

        co_string_token_cleanup(tokens, count);
    }

    return query_map;
}

char*
co_url_query_to_string(
    const co_string_map_t* query_map,
    bool escape
)
{
    co_byte_array_t* buffer = co_byte_array_create();

    co_string_map_const_iterator_t it;
    co_string_map_const_iterator_init(query_map, &it);

    while (co_string_map_const_iterator_has_next(&it))
    {
        const co_string_map_data_st* data =
            co_string_map_const_iterator_get_next(&it);

        if (escape)
        {
            char* key = NULL;
            size_t key_length = 0;

            co_url_component_encode(
                data->key, strlen(data->key), &key, &key_length);

            char* value = NULL;
            size_t value_length = 0;

            co_url_component_encode(
                data->value, strlen(data->value), &value, &value_length);

            co_byte_array_add_string(buffer, key);
            co_byte_array_add_string(buffer, "=");
            co_byte_array_add_string(buffer, value);
            co_byte_array_add_string(buffer, "&");

            co_string_destroy(key);
            co_string_destroy(value);
        }
        else
        {
            co_byte_array_add_string(buffer, data->key);
            co_byte_array_add_string(buffer, "=");
            co_byte_array_add_string(buffer, data->value);
            co_byte_array_add_string(buffer, "&");
        }
    }

    uint8_t* str = co_byte_array_get_ptr(buffer, 0);
    size_t length = co_byte_array_get_count(buffer);

    if (length > 0)
    {
        str[length - 1] = '\0';
    }
    else
    {
        co_byte_array_add_string(buffer, "\0");
    }

    char* result = (char*)co_byte_array_detach(buffer);

    co_byte_array_destroy(buffer);

    return result;
}

bool
co_url_to_net_addr(
    const co_url_st* url,
    int address_family,
    co_net_addr_t* net_addr
)
{
    co_resolve_hint_st hint = { 0 };
    hint.family = address_family;

    if (url->port == 0)
    {
        if (co_net_addr_resolve(
            url->host, url->scheme,
            &hint, net_addr, 1) == 0)
        {
            return false;
        }
    }
    else
    {
        char service[8];
        sprintf(service, "%d", url->port);

        hint.flags |= AI_NUMERICSERV;

        if (co_net_addr_resolve(
            url->host, service,
            &hint, net_addr, 1) == 0)
        {
            return false;
        }
    }

    return true;
}
