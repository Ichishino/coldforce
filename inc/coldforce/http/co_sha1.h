#ifndef CO_SHA1_H_INCLUDED
#define CO_SHA1_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// sha1
//---------------------------------------------------------------------------//

CO_HTTP_API void co_sha1(
    const void* data, uint32_t data_size,
    uint8_t* hash);

#define CO_SHA1_HASH_SIZE   20
#define co_sha1_destroy     co_mem_free

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SHA1_H_INCLUDED
