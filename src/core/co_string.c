#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <ctype.h>

//---------------------------------------------------------------------------//
// string
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
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
co_string_wrap_quotes(
    const char* str,
    bool double_or_single
)
{
    size_t length = strlen(str) + 3;

    char* dest = (char*)co_mem_alloc(length);

    if (dest != NULL)
    {
        strcpy(&dest[1], str);

        const char quote = double_or_single ? '\"' : '\'';

        dest[0] = quote;
        dest[length - 2] = quote;
        dest[length - 1] = '\0';
    }

    return dest;
}

void
co_string_trim_quotes(
    char* str
)
{
    size_t length = strlen(str);

    if (length >= 2)
    {
        if ((str[0] == '\"' && str[length - 1] == '\"') ||
            (str[0] == '\'' && str[length - 1] == '\''))
        {
            memmove(str, &str[1], length - 1);

            str[length - 2] = '\0';
        }
    }
}

char*
co_string_duplicate(
    const char* str
)
{
    size_t length = strlen(str) + 1;

    char* dest = (char*)co_mem_alloc(length);

    if (dest != NULL)
    {
        strcpy(dest, str);
    }

    return dest;
}

char*
co_string_duplicate_n(
    const char* str,
    size_t length
)
{
    char* dest = (char*)co_mem_alloc(length + 1);

    if (dest != NULL)
    {
        strncpy(dest, str, length);
        dest[length] = '\0';
    }

    return dest;
}

char*
co_string_find_n(
    const char* str1,
    const char* str2,
    size_t length
)
{
    char ch2 = *str2;

    if (ch2 != '\0')
    {
        ++str2;

        size_t length2 = strlen(str2);

        do
        {
            char ch1;

            do
            {
                ch1 = *str1;

                if ((length < 1) || (ch1 == '\0'))
                {
                    return NULL;
                }

                ++str1;
                --length;

            } while (ch1 != ch2);

            if (length2 > length)
            {
                return NULL;
            }

        }  while (strncmp(str1, str2, length2) != 0);

        --str1;
    }

    return (char*)str1;
}

void
co_string_hex(
    const void* binary,
    size_t size,
    char* buffer,
    bool uppercase
)
{
    if (uppercase)
    {
        for (size_t index = 0; index < size; ++index)
        {
            sprintf(buffer, "%02X", ((const uint8_t*)binary)[index]);
            buffer += 2;
        }
    }
    else
    {
        for (size_t index = 0; index < size; ++index)
        {
            sprintf(buffer, "%02x", ((const uint8_t*)binary)[index]);
            buffer += 2;
        }
    }
}
