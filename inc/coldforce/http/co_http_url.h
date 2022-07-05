#ifndef CO_HTTP_URL_H_INCLUDED
#define CO_HTTP_URL_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http url
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
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
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_url_st*
co_http_url_create(
    const char* str
);

CO_HTTP_API
void
co_http_url_destroy(
    co_http_url_st* url
);

CO_HTTP_API
char*
co_http_url_create_base_url(
    const co_http_url_st* url
);

CO_HTTP_API
char*
co_http_url_create_host_and_port(
    const co_http_url_st* url
);

CO_HTTP_API
char*
co_http_url_create_path_and_query(
    const co_http_url_st* url
);

CO_HTTP_API
char*
co_http_url_create_file_name(
    const co_http_url_st* url
);

CO_HTTP_API
bool
co_http_url_component_encode(
    const char* src,
    size_t src_length,
    char** dest,
    size_t* dest_length
);

CO_HTTP_API
bool
co_http_url_component_decode(
    const char* src,
    size_t src_length,
    char** dest,
    size_t* dest_length
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_URL_H_INCLUDED
