#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_string_list.h>

//---------------------------------------------------------------------------//
// http string list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

static size_t
co_http_string_cspn(
    const char* str,
    const char* delimiters
)
{
    size_t del_count = strlen(delimiters);

    bool quoted1 = false;
    bool quoted2 = false;

    const char* ptr = str;

    while ((*ptr) != '\0')
    {
        if (!quoted2 && (*ptr) == '\"')
        {
            quoted1 = !quoted1;
        }

        if (!quoted1 && (*ptr) == '\'')
        {
            quoted2 = !quoted2;
        }

        if (!quoted1 && !quoted2)
        {
            for (size_t i = 0; i < del_count; ++i)
            {
                if ((*ptr) == delimiters[i])
                {
                    return (size_t)(ptr - str);
                }
            }
        }

        ++ptr;
    }

    return (size_t)(ptr - str);
}

size_t
co_http_string_list_parse(
    const char* str,
    co_http_string_item_st* items,
    size_t count
)
{
    static const char* delimiter = ";&, ";

    size_t index = 0;

    while (((*str) != '\0') && (index < count))
    {
        size_t length = co_http_string_cspn(str, delimiter);

        if (length == 0)
        {
            ++str;

            continue;
        }

        char* ptr = co_string_find_n(str, "=", length);

        if (ptr != NULL)
        {
            size_t first_length = ptr - str;
            size_t second_length = length - (first_length + 1);

            items[index].first =
                co_string_duplicate_n(str, first_length);

            co_string_trim(items[index].first, first_length);
            co_string_trim_quotes(items[index].first);

            if (second_length > 0)
            {
                items[index].second =
                    co_string_duplicate_n(&str[first_length + 1], second_length);

                co_string_trim(items[index].second, second_length);
                co_string_trim_quotes(items[index].second);
            }
            else
            {
                items[index].second = NULL;
            }
        }
        else
        {
            items[index].first =
                co_string_duplicate_n(str, length);
            items[index].second = NULL;

            co_string_trim(items[index].first, length);

            co_string_trim_quotes(items[index].first);
        }

        str += length;
        ++index;
    }

    return index;
}

void
co_http_string_list_cleanup(
    co_http_string_item_st* items,
    size_t count
)
{
    for (size_t index = 0; index < count; ++index)
    {
        co_string_destroy(items[index].first);
        items[index].first = NULL;

        co_string_destroy(items[index].second);
        items[index].second = NULL;
    }
}

int
co_http_string_list_find(
    const co_http_string_item_st* items,
    size_t item_count,
    const char* first
)
{
    for (size_t index = 0; index < item_count; ++index)
    {
        if (items[index].first != NULL &&
            co_string_case_compare(items[index].first, first) == 0)
        {
            return (int)index;
        }
    }

    return -1;
}

bool
co_http_string_list_contains(
    const co_http_string_item_st* items,
    size_t item_count,
    const char* first
)
{
    for (size_t index = 0; index < item_count; ++index)
    {
        if (items[index].first != NULL &&
            co_string_case_compare(items[index].first, first) == 0)
        {
            return true;
        }
    }

    return false;
}
