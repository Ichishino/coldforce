#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>
#include <coldforce/core/co_random.h>

#ifdef CO_OS_WIN
#include <windows.h>
#endif

//---------------------------------------------------------------------------//
// random generator
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_random(
    void* buffer,
    size_t length
)
{
#ifdef CO_OS_WIN
    HCRYPTPROV prov;
    CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, 0);
    CryptGenRandom(prov, (DWORD)length, (BYTE*)buffer);
    CryptReleaseContext(prov, 0);
#else
    for (size_t index = 0; index < length; ++index)
    {
        ((uint8_t*)buffer)[index] = (uint8_t)(random() % 256);
    }
#endif
}

uint32_t
co_random_range(
    uint32_t min,
    uint32_t max
)
{
    uint32_t value;
    co_random(&value, sizeof(value));

    return (uint32_t)((value % (max - min + 1)) + min);
}

void
co_random_string(
    char* buffer,
    size_t length,
    const char* characters
)
{
    uint32_t characters_length =
        (uint32_t)strlen(characters);

    for (size_t index = 0; index < length; ++index)
    {
        buffer[index] = characters[
            co_random_range(0, characters_length - 1)];
    }

    buffer[length] = '\0';
}

void
co_random_alnum_string(
    char* buffer,
    size_t length
)
{
    static const char* alnum =
        "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    co_random_string(buffer, length, alnum);
}

void
co_random_hex_string(
    char* buffer,
    size_t length
)
{
    static const char* hex =
        "0123456789abcdef";

    co_random_string(buffer, length, hex);
}
