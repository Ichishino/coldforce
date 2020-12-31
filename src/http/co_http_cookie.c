#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_cookie.h>

//---------------------------------------------------------------------------//
// http cookie
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_request_cookie_serialize(
    const co_http_cookie_st* cookie,
    size_t count,
    co_byte_array_t* buffer
)
{
    if (count == 0)
    {
        return;
    }

    if (co_byte_array_get_count(buffer) > 0)
    {
        co_byte_array_add_string(buffer, "; ");
    }

    for (size_t index = 0; index < count; ++index)
    {
        co_byte_array_add_string(buffer, cookie[index].name);
        co_byte_array_add_string(buffer, "=");
        co_byte_array_add_string(buffer, cookie[index].value);
        co_byte_array_add_string(buffer, "; ");
    }

    co_byte_array_set_count(
        buffer, co_byte_array_get_count(buffer) - 2);
}

size_t
co_http_request_cookie_deserialize(
    const char* str,
    co_http_cookie_st* cookie,
    size_t count
)
{
    size_t length = strlen(str);

    size_t index = 0;

    while ((*str) != '\0' && (index < count))
    {
        while ((*str) == ' ')
        {
            ++str;
            --length;
        }

        const char* sc = co_string_find_n(str, ";", length);

        if (sc == NULL)
        {
            sc = str + length;
        }

        size_t item_length = sc - str;

        const char* eq = co_string_find_n(str, "=", item_length);

        if (eq == NULL)
        {
            return index;
        }

        size_t name_length = eq - str;
        size_t value_length = sc - eq - 1;

        memset(&cookie[index], 0x00, sizeof(co_http_cookie_st));

        cookie[index].name = co_string_duplicate_n(str, name_length);
        cookie[index].value = co_string_duplicate_n(eq + 1, value_length);

        co_string_trim(cookie->name, name_length);
        co_string_trim(cookie->value, value_length);

        ++index;

        str = sc;
        ++str;
        length -= (item_length + 1);
    }

    return index;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_response_cookie_serialize(
    const co_http_cookie_st* cookie,
    co_byte_array_t* buffer
)
{
    co_byte_array_add_string(buffer, cookie->name);
    co_byte_array_add_string(buffer, "=");
    co_byte_array_add_string(buffer, cookie->value);

    if (cookie->attr.expires != NULL)
    {
        co_byte_array_add_string(buffer, "; Expires=");
        co_byte_array_add_string(buffer, cookie->attr.expires);
    }

    if (cookie->attr.path != NULL)
    {
        co_byte_array_add_string(buffer, "; Path=");
        co_byte_array_add_string(buffer, cookie->attr.path);
    }

    if (cookie->attr.domain != NULL)
    {
        co_byte_array_add_string(buffer, "; Domain=");
        co_byte_array_add_string(buffer, cookie->attr.domain);
    }

    if (cookie->attr.max_age != NULL)
    {
        co_byte_array_add_string(buffer, "; Max-Age=");
        co_byte_array_add_string(buffer, cookie->attr.max_age);
    }

    if (cookie->attr.priority != NULL)
    {
        co_byte_array_add_string(buffer, "; Priority=");
        co_byte_array_add_string(buffer, cookie->attr.priority);
    }

    if (cookie->attr.same_site != NULL)
    {
        co_byte_array_add_string(buffer, "; SameSite=");
        co_byte_array_add_string(buffer, cookie->attr.same_site);
    }

    if (cookie->attr.version != NULL)
    {
        co_byte_array_add_string(buffer, "; Version=");
        co_byte_array_add_string(buffer, cookie->attr.version);
    }

    if (cookie->attr.comment != NULL)
    {
        co_byte_array_add_string(buffer, "; Comment=");
        co_byte_array_add_string(buffer, cookie->attr.comment);
    }

    if (cookie->attr.secure)
    {
        co_byte_array_add_string(buffer, "; Secure");
    }

    if (cookie->attr.http_only)
    {
        co_byte_array_add_string(buffer, "; HttpOnly");
    }
}

bool
co_http_response_cookie_deserialize(
    const char* str,
    co_http_cookie_st* cookie
)
{
    memset(cookie, 0x00, sizeof(co_http_cookie_st));

    size_t length = strlen(str);

    const char* sc = co_string_find_n(str, ";", length);

    if (sc == NULL)
    {
        return false;
    }

    size_t item_length = sc - str;

    const char* eq = co_string_find_n(str, "=", item_length);

    if (eq == NULL)
    {
        return false;
    }

    size_t name_length = eq - str;
    size_t value_length = sc - eq - 1;

    cookie->name = co_string_duplicate_n(str, name_length);
    cookie->value = co_string_duplicate_n(eq + 1, value_length);

    co_string_trim(cookie->name, name_length);
    co_string_trim(cookie->value, value_length);

    str = sc;
    ++str;
    length -= (item_length + 1);

    while ((((int)length) > 0) && ((*str) != '\0'))
    {
        while ((*str) == ' ')
        {
            ++str;
            --length;
        }

        sc = co_string_find_n(str, ";", length);

        if (sc == NULL)
        {
            sc = str + length;
        }

        item_length = sc - str;

        eq = co_string_find_n(str, "=", item_length);

        if (eq != NULL)
        {
            name_length = eq - str;
            value_length = sc - eq - 1;

            char* name = co_string_duplicate_n(str, name_length);
            char* value = co_string_duplicate_n(eq + 1, value_length);

            co_string_trim(name, name_length);
            co_string_trim(value, value_length);

            if (co_string_case_compare_n(name, "Expires", name_length) == 0)
            {
                cookie->attr.expires = value;
            }
            else if (co_string_case_compare_n(name, "Path", name_length) == 0)
            {
                cookie->attr.path = value;
            }
            else if (co_string_case_compare_n(name, "Domain", name_length) == 0)
            {
                cookie->attr.domain = value;
            }
            else if (co_string_case_compare_n(name, "Max-Age", name_length) == 0)
            {
                cookie->attr.max_age = value;
            }
            else if (co_string_case_compare_n(name, "Priority", name_length) == 0)
            {
                cookie->attr.priority = value;
            }
            else if (co_string_case_compare_n(name, "SameSite", name_length) == 0)
            {
                cookie->attr.same_site = value;
            }
            else if (co_string_case_compare_n(name, "Version", name_length) == 0)
            {
                cookie->attr.version = value;
            }
            else if (co_string_case_compare_n(name, "Comment", name_length) == 0)
            {
                cookie->attr.comment = value;
            }
            else
            {
                char* item = co_string_duplicate_n(str, item_length);

                printf("Unknown Cookie: [%s]\n", item);

                co_string_destroy(item);
                co_string_destroy(value);
            }

            co_string_destroy(name);
        }
        else
        {
            char* item = co_string_duplicate_n(str, item_length);

            co_string_trim(item, item_length);

            if (co_string_case_compare_n(item, "Secure", item_length) == 0)
            {
                cookie->attr.secure = true;
            }
            else if (co_string_case_compare_n(item, "HttpOnly", item_length) == 0)
            {
                cookie->attr.http_only = true;
            }
            else
            {
                printf("Unknown Cookie: [%s]\n", item);
            }

            co_string_destroy(item);
        }

        str = sc;
        ++str;
        length -= (item_length + 1);
    }

    return true;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_cookie_cleanup(
    co_http_cookie_st* cookies,
    size_t count
)
{
    for (size_t index = 0; index < count; ++index)
    {
        co_string_destroy(cookies[index].name);
        co_string_destroy(cookies[index].value);
        co_string_destroy(cookies[index].attr.expires);
        co_string_destroy(cookies[index].attr.path);
        co_string_destroy(cookies[index].attr.domain);
        co_string_destroy(cookies[index].attr.max_age);
        co_string_destroy(cookies[index].attr.priority);
        co_string_destroy(cookies[index].attr.same_site);
        co_string_destroy(cookies[index].attr.version);
        co_string_destroy(cookies[index].attr.comment);
    }
}
