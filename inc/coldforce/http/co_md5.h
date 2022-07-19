#ifndef CO_MD5_H_INCLUDED
#define CO_MD5_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// md5
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];

} co_md5_ctx_t;

#define CO_MD5_HASH_SIZE   16

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
void
co_md5_init(
    co_md5_ctx_t* ctx
);

CO_HTTP_API
void
co_md5_update(
    co_md5_ctx_t* ctx,
    const void* data,
    uint32_t data_size
);

CO_HTTP_API
void
co_md5_final(
    co_md5_ctx_t* ctx,
    uint8_t* hash
);

CO_HTTP_API
void
co_md5(
    const void* data,
    uint32_t data_size,
    uint8_t* hash
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_MD5_H_INCLUDED
