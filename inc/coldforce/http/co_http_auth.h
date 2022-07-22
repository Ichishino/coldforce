#ifndef CO_HTTP_AUTH_H_INCLUDED
#define CO_HTTP_AUTH_H_INCLUDED

#include <coldforce/core/co_ss_map.h>

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

struct co_http_response_t;

//---------------------------------------------------------------------------//
// http auth
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    char* scheme;
    char* method;
    char* credentials;
    co_map_t* items;

} co_http_auth_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

char*
co_http_auth_serialize_request(
    const co_http_auth_t* auth
);

bool
co_http_auth_deserialize_request(
    const char* str,
    co_http_auth_t* auth
);

char*
co_http_auth_serialize_response(
    const co_http_auth_t* auth
);

bool
co_http_auth_deserialize_response(
    const char* str,
    co_http_auth_t* auth
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_HTTP_API
char*
co_http_auth_create_basic_credentials(
    const char* user,
    const char* password
);

CO_HTTP_API
bool
co_http_auth_parse_basic_credentials(
    const char* credentials,
    char** user,
    char** password
);

CO_HTTP_API
char*
co_http_auth_create_digest_credentials(
    const char* user,
    const char* password,
    const co_http_auth_t* response_auth
);

CO_HTTP_API
co_http_auth_t*
co_http_auth_create(
    void
);

CO_HTTP_API
co_http_auth_t*
co_http_auth_create_basic_request(
    const char* credentials
);

CO_HTTP_API
co_http_auth_t*
co_http_auth_create_digest_request(
    const char* method,
    const char* path,
    const char* user,
    const char* credentials,
    const char* cnonce,
    uint32_t nc,
    const co_http_auth_t* response_auth
);

CO_HTTP_API
co_http_auth_t*
co_http_auth_create_response(
    const char* header_name,
    const struct co_http_response_t* response
);

CO_HTTP_API
void
co_http_auth_destroy(
    co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_auth_set_scheme(
    co_http_auth_t* auth,
    const char* scheme
);

CO_HTTP_API
const char*
co_http_auth_get_scheme(
    const co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_auth_set_method(
    co_http_auth_t* auth,
    const char* method
);

CO_HTTP_API
const char*
co_http_auth_get_method(
    const co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_auth_set_value(
    co_http_auth_t* auth,
    const char* name,
    const char* value
);

CO_HTTP_API
const void*
co_http_auth_get_value(
    const co_http_auth_t* auth,
    const char* name
);

CO_HTTP_API
void
co_http_auth_set_credentials(
    co_http_auth_t* auth,
    const char* credentials
);

CO_HTTP_API
const char*
co_http_auth_get_credentials(
    const co_http_auth_t* auth
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_AUTH_H_INCLUDED
