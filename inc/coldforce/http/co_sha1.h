#ifndef CO_SHA1_H_INCLUDED
#define CO_SHA1_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// sha1
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];

} co_sha1_ctx_t;

#define CO_SHA1_HASH_SIZE   20

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
void
co_sha1_init(
    co_sha1_ctx_t* ctx
);

CO_HTTP_API
void
co_sha1_update(
    co_sha1_ctx_t* ctx,
    const void* data,
    uint32_t data_size
);

CO_HTTP_API
void
co_sha1_final(
    co_sha1_ctx_t* ctx,
    uint8_t* hash
);

CO_HTTP_API
void
co_sha1(
    const void* data,
    uint32_t data_size,
    uint8_t* hash
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SHA1_H_INCLUDED
