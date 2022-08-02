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
    size_t data_size
)
{
#ifdef CO_OS_WIN
    HCRYPTPROV prov;
    CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, 0);
    CryptGenRandom(prov, (DWORD)data_size, (BYTE*)buffer);
    CryptReleaseContext(prov, 0);
#else
    for (size_t index = 0; index < data_size; ++index)
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
co_random_hex_string(
    char* buffer,
    size_t length
)
{
    size_t bin_size = length / 2;

    if (length % 2)
    {
        bin_size += 1;
    }

    uint8_t* bin =
        (uint8_t*)co_mem_alloc(bin_size);
    co_random(bin, bin_size);
    co_string_hex(bin, bin_size, buffer, false);
    co_mem_free(bin);

    buffer[length] = '\0';
}
