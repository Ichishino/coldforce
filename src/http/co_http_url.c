#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_url.h>

#include <ctype.h>

//---------------------------------------------------------------------------//
// http url
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http_url_st*
co_http_url_create(
    const char* str
)
{
    co_http_url_st* url =
        (co_http_url_st*)co_mem_alloc(sizeof(co_http_url_st));

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
co_http_url_destroy(
    co_http_url_st* url
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
co_http_url_create_base_url(
    const co_http_url_st* url
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
co_http_url_create_host_and_port(
    const co_http_url_st* url
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
co_http_url_create_path_and_query(
    const co_http_url_st* url
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
co_http_url_create_file_name(
    const co_http_url_st* url
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
co_http_url_component_encode(
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
        else if (src[index] == ' ')
        {
            (*dest)[(*dest_length)++] = '+';
        }
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
co_http_url_component_decode(
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
