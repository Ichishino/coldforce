#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <ctype.h>

//---------------------------------------------------------------------------//
// string
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

size_t
co_string_hash(
    const char* str
)
{
    // D.J.Bernstein hash

    size_t hash = 5381;

    while ((*str) != 0)
    {
        hash = 33 * hash ^ (unsigned char)*str;
        ++str;
    }

    return hash;
}

size_t
co_string_trim_left(
    char* str,
    size_t length
)
{
    if ((length == 0) || (isspace(str[0]) == 0))
    {
        return length;
    }

    size_t new_length = 0;

    for (size_t index = 1; index < length; ++index)
    {
        if (isspace(str[index]) == 0)
        {
            new_length = length - index;
            memcpy(&str[0], &str[index], new_length);

            break;
        }
    }

    str[new_length] = 0x00;

    return new_length;
}

size_t
co_string_trim_right(
    char* str,
    size_t length
)
{
    if ((length == 0) || (isspace(str[length - 1]) == 0))
    {
        return length;
    }

    size_t new_length = 0;

    for (long index = ((long)length) - 2; index >= 0; --index)
    {
        if (isspace(str[index]) == 0)
        {
            new_length = ((size_t)index) + 1;

            break;
        }
    }

    str[new_length] = 0x00;

    return new_length;
}

size_t
co_string_trim(
    char* str,
    size_t length
)
{
    length = co_string_trim_right(str, length);
    length = co_string_trim_left(str, length);

    return length;
}

char*
co_string_duplicate(
    const char* src
)
{
    size_t length = strlen(src) + 1;

    char* dest = (char*)co_mem_alloc(length);

    if (dest != NULL)
    {
        strcpy(dest, src);
    }

    return dest;
}
