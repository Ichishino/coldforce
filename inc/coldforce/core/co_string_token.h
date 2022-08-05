#ifndef CO_STRING_TOKEN_H_INCLUDED
#define CO_STRING_TOKEN_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string token
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

// ex.
//
// "aaaa=1234&bbb=5678&ccc=9999"
// "aaaa=1234; bbb=5678; ccc=9999"
// "aaaa, bbbb, cccc"

typedef struct
{
    char* first;
    char* second;

} co_string_token_st;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
size_t
co_string_token_split(
    const char* str,
    co_string_token_st* tokens,
    size_t count
);

CO_CORE_API
void
co_string_token_cleanup(
    co_string_token_st* tokens,
    size_t count
);

CO_CORE_API
int
co_string_token_find(
    const co_string_token_st* tokens,
    size_t count,
    const char* first
);

CO_CORE_API
bool
co_string_token_contains(
    const co_string_token_st* token,
    size_t count,
    const char* first
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_TOKEN_H_INCLUDED
