#ifndef CO_HTTP_COOKIE_H_INCLUDED
#define CO_HTTP_COOKIE_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http cookie
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    char* name;
    char* value;

    struct co_http_cookie_attribute_t
    {
        char* expires;
        char* path;
        char* domain;
        char* max_age;
        char* priority;
        char* same_site;
        char* version;
        char* comment;

        bool secure;
        bool http_only;

    } attr;

} co_http_cookie_st;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_HTTP_API
void
co_http_response_cookie_serialize(
    const co_http_cookie_st* cookie,
    co_byte_array_t* buffer
);

CO_HTTP_API
bool
co_http_response_cookie_deserialize(
    const char* str,
    co_http_cookie_st* cookie
);

CO_HTTP_API
void
co_http_request_cookie_serialize(
    const co_http_cookie_st* cookie,
    size_t count,
    co_byte_array_t* buffer
);

CO_HTTP_API
size_t
co_http_request_cookie_deserialize(
    const char* str,
    co_http_cookie_st* cookie,
    size_t count
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
void
co_http_cookie_cleanup(
    co_http_cookie_st* cookies,
    size_t count
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_COOKIE_H_INCLUDED
