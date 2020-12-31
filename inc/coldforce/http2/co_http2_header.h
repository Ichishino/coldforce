#ifndef CO_HTTP2_HEADER_H_INCLUDED
#define CO_HTTP2_HEADER_H_INCLUDED

#include <coldforce/core/co_list.h>

#include <coldforce/http/co_http_url.h>

#include <coldforce/http2/co_http2.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 header
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct co_http2_pseudo_header_t
{
    char* authority;
    char* method;
    co_http_url_st* url;
    char* scheme;
    uint16_t status_code;

} co_http2_pseudo_header_t;

typedef struct co_http2_header_field_t
{
    char* name;
    char* value;

} co_http2_header_field_t;

typedef struct co_http2_header_t
{
    co_http2_pseudo_header_t pseudo;
    co_list_t* field_list;

} co_http2_header_t;

void co_http2_header_setup(co_http2_header_t* header);
void co_http2_header_cleanup(co_http2_header_t* header);

bool co_http2_header_add_field_ptr(
    co_http2_header_t* header, char* name, char* value);

CO_HTTP2_API void co_http2_header_print(const co_http2_header_t* header);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API void co_http2_header_clear(co_http2_header_t* header);

CO_HTTP2_API void co_http2_header_set_authority(co_http2_header_t* header, const char* authority);
CO_HTTP2_API const char* co_http2_header_get_authority(const co_http2_header_t* message);

CO_HTTP2_API void co_http2_header_set_method(co_http2_header_t* header, const char* method);
CO_HTTP2_API const char* co_http2_header_get_method(const co_http2_header_t* header);

CO_HTTP2_API void co_http2_header_set_path(co_http2_header_t* header, const char* path);
CO_HTTP2_API const char* co_http2_header_get_path(const co_http2_header_t* header);
CO_HTTP2_API const co_http_url_st* co_http2_header_get_url(const co_http2_header_t* header);

CO_HTTP2_API void co_http2_header_set_scheme(co_http2_header_t* header, const char* scheme);
CO_HTTP2_API const char* co_http2_header_get_scheme(const co_http2_header_t* header);

CO_HTTP2_API void co_http2_header_set_status_code(co_http2_header_t* header, uint16_t status_code);
CO_HTTP2_API uint16_t co_http2_header_get_status_code(const co_http2_header_t* header);

CO_HTTP2_API size_t co_http2_header_get_field_count(const co_http2_header_t* header);
CO_HTTP2_API size_t co_http2_header_get_value_count(const co_http2_header_t* header, const char* name);
CO_HTTP2_API bool co_http2_header_contains(const co_http2_header_t* header, const char* name);

CO_HTTP2_API void co_http2_header_set_field(
    co_http2_header_t* header, const char* key, const char* value);
CO_HTTP2_API const char* co_http2_header_get_field(
    const co_http2_header_t* header, const char* name);
CO_HTTP2_API size_t co_http2_header_get_fields(
    const co_http2_header_t* header, const char* name,
    const char* value[], size_t count);
CO_HTTP2_API bool co_http2_header_add_field(
    co_http2_header_t* header, const char* name, const char* value);

CO_HTTP2_API void co_http2_header_remove_field(co_http2_header_t* header, const char* name);
CO_HTTP2_API void co_http2_header_remove_all_fields(co_http2_header_t* header, const char* name);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_HEADER_H_INCLUDED
