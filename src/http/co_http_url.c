#include <coldforce/core/co_std.h>
#include <coldforce/core/co_byte_array.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_url.h>

//---------------------------------------------------------------------------//
// http url
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
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

    ptr = strchr(head, '@');

    if (ptr != NULL)
    {
        char* sep = strchr(head, ':');

        if (sep != NULL)
        {
            url->user =
                co_string_duplicate_n(head, sep - head);

            head = sep + 1;
        }

        url->password = 
            co_string_duplicate_n(head, ptr - head);

        head = ptr + 1;
    }

    ptr = strchr(head, '/');

    if (ptr != NULL)
    {
        url->path = co_string_duplicate(ptr);

        head[ptr - head] = '\0';
    }

    ptr = strchr(head, ':');

    if (ptr != NULL)
    {
        url->host = co_string_duplicate_n(head, ptr - head);

        if (strlen(ptr + 1) <= 5)
        {
            url->port = (uint16_t)strtol(ptr + 1, NULL, 10);
        }
    }
    else
    {
        url->host = co_string_duplicate(head);
    }

    co_mem_free(url_str);

    return url;
}

void
co_http_url_destroy(
    co_http_url_st* url
)
{
    if (url != NULL)
    {
        co_mem_free(url->src);
        co_mem_free(url->scheme);
        co_mem_free(url->user);
        co_mem_free(url->password);
        co_mem_free(url->host);
        co_mem_free(url->path);
        co_mem_free(url->query);
        co_mem_free(url->fragment);

        co_mem_free(url);
    }
}
