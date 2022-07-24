#ifndef CO_HTTP_AUTH_H_INCLUDED
#define CO_HTTP_AUTH_H_INCLUDED

#include <coldforce/core/co_ss_map.h>

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

struct co_http_request_t;
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
co_http_auth_t*
co_http_auth_create(
    void
);

CO_HTTP_API
co_http_auth_t*
co_http_auth_create_request(
    const char* header_name,
    const struct co_http_request_t* request
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
co_http_auth_set_item(
    co_http_auth_t* auth,
    const char* name,
    const char* value
);

CO_HTTP_API
const char*
co_http_auth_get_item(
    const co_http_auth_t* auth,
    const char* name
);

//---------------------------------------------------------------------------//
// basic auth
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_auth_t*
co_http_basic_auth_create_request(
    const char* user,
    const char* password
);

CO_HTTP_API
co_http_auth_t*
co_http_basic_auth_create_response(
    const char* realm
);

CO_HTTP_API
bool
co_http_basic_auth_get_credentials(
    const co_http_auth_t* auth,
    char** user,
    char** password
);

//---------------------------------------------------------------------------//
// digest auth
//---------------------------------------------------------------------------//

CO_HTTP_API
co_http_auth_t*
co_http_digest_auth_create_request(
    const char* user,
    const char* password,
    const co_http_auth_t* response_auth
);

CO_HTTP_API
co_http_auth_t*
co_http_digest_auth_create_response(
    const char* realm,
    const char* nonce,
    const char* opaque
);

CO_HTTP_API
bool
co_http_digest_auth_validate(
    const co_http_auth_t* request_auth,
    const char* method,
    const char* path,
    const char* realm,
    const char* user,
    const char* password,
    const char* nonce,
    uint32_t nc
);

CO_HTTP_API
void
co_http_digest_auth_set_method(
    co_http_auth_t* auth,
    const char* method
);

CO_HTTP_API
const char*
co_http_digest_auth_get_method(
    const co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_digest_auth_set_path(
    co_http_auth_t* auth,
    const char* path
);

CO_HTTP_API
const char*
co_http_digest_auth_get_path(
    const co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_digest_auth_set_user(
    co_http_auth_t* auth,
    const char* user
);

CO_HTTP_API
const char*
co_http_digest_auth_get_user(
    const co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_digest_auth_set_count(
    co_http_auth_t* auth,
    uint32_t count
);

CO_HTTP_API
uint32_t
co_http_digest_auth_get_count(
    const co_http_auth_t* auth
);

CO_HTTP_API
void
co_http_digest_auth_set_opaque(
    co_http_auth_t* auth,
    const char* opaque
);

CO_HTTP_API
const char*
co_http_digest_auth_get_opaque(
    const co_http_auth_t* auth
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_AUTH_H_INCLUDED
