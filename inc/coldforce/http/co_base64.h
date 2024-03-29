#ifndef CO_BASE64_H_INCLUDED
#define CO_BASE64_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// base64
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
void
co_base64_encode(
    const void* src,
    size_t src_length,
    char** dest,
    size_t* dest_length,
    bool padding
);

CO_HTTP_API
bool
co_base64_decode(
    const char* src,
    size_t src_length,
    uint8_t** dest,
    size_t* dest_length
);

CO_HTTP_API
void
co_base64url_encode(
    const void* src,
    size_t src_length,
    char** dest,
    size_t* dest_length,
    bool padding
);

CO_HTTP_API
bool
co_base64url_decode(
    const char* src,
    size_t src_length,
    uint8_t** dest,
    size_t* dest_length
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_BASE64_H_INCLUDED
