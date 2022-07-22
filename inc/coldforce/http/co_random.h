#ifndef CO_RANDOM_H_INCLUDED
#define CO_RANDOM_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// random generator
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
void
co_random(
    void* buffer,
    size_t data_size
);

CO_HTTP_API
void
co_random_hex_string(
    char* buffer,
    size_t length
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_RANDOM_H_INCLUDED