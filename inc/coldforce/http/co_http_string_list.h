#ifndef CO_HTTP_STRING_LIST_H_INCLUDED
#define CO_HTTP_STRING_LIST_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http string list
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

} co_http_string_item_st;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
size_t
co_http_string_list_parse(
    const char* str,
    co_http_string_item_st* items,
    size_t count
);

CO_HTTP_API
void
co_http_string_list_cleanup(
    co_http_string_item_st* items,
    size_t count
);

CO_HTTP_API
const char*
co_http_string_list_find(
    const co_http_string_item_st* items,
    size_t item_count,
    const char* first
);

CO_HTTP_API
bool
co_http_string_list_contains(
    const co_http_string_item_st* items,
    size_t item_count,
    const char* first
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_STRING_LIST_H_INCLUDED
