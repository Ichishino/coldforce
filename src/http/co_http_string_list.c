#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_string_list.h>

//---------------------------------------------------------------------------//
// http string list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

size_t
co_http_string_list_parse(
    const char* str,
    co_http_string_item_st* items,
    size_t count
)
{
    static const char* delimiter = ";&,";

    size_t index = 0;

    while (((*str) != '\0') && (index < count))
    {
        size_t length = strcspn(str, delimiter);

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
            items[index].second =
                co_string_duplicate_n(&str[first_length + 1], second_length);

            co_string_trim(items[index].first, first_length);
            co_string_trim(items[index].second, second_length);
        }
        else
        {
            items[index].first =
                co_string_duplicate_n(str, length);
            items[index].second = NULL;

            co_string_trim(items[index].first, length);
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
        co_mem_free(items[index].first);
        items[index].first = NULL;

        co_mem_free(items[index].second);
        items[index].second = NULL;
    }
}

const char*
co_http_string_list_find(
    co_http_string_item_st* items,
    size_t item_count,
    const char* first
)
{
    for (size_t index = 0; index < item_count; ++index)
    {
        if (co_string_case_compare(items[index].first, first) == 0)
        {
            return items[index].second;
        }
    }

    return NULL;
}

bool
co_http_string_list_contains(
    co_http_string_item_st* items,
    size_t item_count,
    const char* first
)
{
    for (size_t index = 0; index < item_count; ++index)
    {
        if (co_string_case_compare(items[index].first, first) == 0)
        {
            return true;
        }
    }

    return false;
}
