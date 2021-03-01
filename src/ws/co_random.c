#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_random.h>

#ifdef CO_OS_WIN
#include <windows.h>
#endif

//---------------------------------------------------------------------------//
// random generator
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
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
