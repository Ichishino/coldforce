#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>
#include <coldforce/core/co_string_token.h>

//---------------------------------------------------------------------------//
// string token
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static size_t
co_string_token_cspn(
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

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

size_t
co_string_token_split(
    const char* str,
    co_string_token_st* tokens,
    size_t count
)
{
    static const char* delimiter = ";&, ";

    size_t index = 0;

    while (((*str) != '\0') && (index < count))
    {
        size_t length = co_string_token_cspn(str, delimiter);

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

            tokens[index].first =
                co_string_duplicate_n(str, first_length);

            co_string_trim(tokens[index].first, first_length);
            co_string_trim_quotes(tokens[index].first);

            if (second_length > 0)
            {
                tokens[index].second =
                    co_string_duplicate_n(&str[first_length + 1], second_length);

                co_string_trim(tokens[index].second, second_length);
                co_string_trim_quotes(tokens[index].second);
            }
            else
            {
                tokens[index].second = NULL;
            }
        }
        else
        {
            tokens[index].first =
                co_string_duplicate_n(str, length);
            tokens[index].second = NULL;

            co_string_trim(tokens[index].first, length);

            co_string_trim_quotes(tokens[index].first);
        }

        str += length;
        ++index;
    }

    return index;
}

void
co_string_token_cleanup(
    co_string_token_st* tokens,
    size_t count
)
{
    for (size_t index = 0; index < count; ++index)
    {
        co_string_destroy(tokens[index].first);
        tokens[index].first = NULL;

        co_string_destroy(tokens[index].second);
        tokens[index].second = NULL;
    }
}

int
co_string_token_find(
    const co_string_token_st* tokens,
    size_t count,
    const char* first
)
{
    for (size_t index = 0; index < count; ++index)
    {
        if (tokens[index].first != NULL &&
            co_string_case_compare(tokens[index].first, first) == 0)
        {
            return (int)index;
        }
    }

    return -1;
}

bool
co_string_token_contains(
    const co_string_token_st* tokens,
    size_t count,
    const char* first
)
{
    for (size_t index = 0; index < count; ++index)
    {
        if (tokens[index].first != NULL &&
            co_string_case_compare(tokens[index].first, first) == 0)
        {
            return true;
        }
    }

    return false;
}
