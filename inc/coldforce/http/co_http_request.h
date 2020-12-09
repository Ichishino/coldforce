#ifndef CO_HTTP_REQUEST_H_INCLUDED
#define CO_HTTP_REQUEST_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_url.h>
#include <coldforce/http/co_http_message.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http request
//---------------------------------------------------------------------------//

typedef struct
{
    co_http_message_t message;

    co_http_url_st* url;

    char* method;
    char* version;

    char* save_file_path;

} co_http_request_t;

void co_http_request_serialize(
    const co_http_request_t* request, co_byte_array_t* buffer);
int co_http_request_deserialize(
    co_http_request_t* request, const co_byte_array_t* data, size_t* index);

CO_HTTP_API void co_http_request_print_header(const co_http_request_t* request, FILE* fp);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_request_t* co_http_request_create(const char* method, const char* url_str);
CO_HTTP_API void co_http_request_destroy(co_http_request_t* request);

CO_HTTP_API co_http_header_t* co_http_request_get_header(co_http_request_t* request);
CO_HTTP_API const co_http_header_t* co_http_request_get_const_header(const co_http_request_t* request);

CO_HTTP_API bool co_http_request_set_content(
    co_http_request_t* request, const void* data, size_t data_size);

CO_HTTP_API const void* co_http_request_get_content(const co_http_request_t* request);
CO_HTTP_API size_t co_http_request_get_content_size(const co_http_request_t* request);

CO_HTTP_API void co_http_request_set_url(co_http_request_t* request, const char* url_str);
CO_HTTP_API const char* co_http_request_get_url(const co_http_request_t* request);

CO_HTTP_API void co_http_request_set_method(co_http_request_t* request, const char* method);
CO_HTTP_API const char* co_http_request_get_method(const co_http_request_t* request);

CO_HTTP_API void co_http_request_set_version(co_http_request_t* request, const char* version);
CO_HTTP_API const char* co_http_request_get_version(const co_http_request_t* request);

CO_HTTP_API void co_http_request_set_save_file_path(
    co_http_request_t* request, const char* save_file_path);
CO_HTTP_API const char* co_http_request_get_save_file_path(
    const co_http_request_t* request);

CO_HTTP_API void co_http_request_set_cookies(
    co_http_request_t* request, const co_http_cookie_st* cookies, size_t count);
CO_HTTP_API void co_http_request_add_cookie(
    co_http_request_t* request, const co_http_cookie_st* cookie);
CO_HTTP_API size_t co_http_request_get_cookies(
    const co_http_request_t* request, co_http_cookie_st* cookies, size_t count);
CO_HTTP_API void co_http_request_remove_all_cookies(
    co_http_request_t* request);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_REQUEST_H_INCLUDED
