#ifndef CO_RANDOM_H_INCLUDED
#define CO_RANDOM_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// random generator
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
void
co_random(
    void* buffer,
    size_t length
);

CO_CORE_API
uint32_t
co_random_range(
    uint32_t min,
    uint32_t max
);

CO_CORE_API
void
co_random_string(
    char* buffer,
    size_t length,
    const char* characters
);

CO_CORE_API
void
co_random_alnum_string(
    char* buffer,
    size_t length
);

CO_CORE_API
void
co_random_hex_string(
    char* buffer,
    size_t length
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_RANDOM_H_INCLUDED
