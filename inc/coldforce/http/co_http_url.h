#ifndef CO_HTTP_URL_H_INCLUDED
#define CO_HTTP_URL_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http url
//---------------------------------------------------------------------------//

typedef struct
{
    char* src;
    char* scheme;
    char* user;
    char* password;
    char* host;
    uint16_t port;
    char* path;
    char* query;
    char* fragment;

} co_http_url_st;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_url_st* co_http_url_create(const char* str);
CO_HTTP_API void co_http_url_destroy(co_http_url_st* url);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_URL_H_INCLUDED
