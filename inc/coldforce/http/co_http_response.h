#ifndef CO_HTTP_RESPONSE_H_INCLUDED
#define CO_HTTP_RESPONSE_H_INCLUDED

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_message.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http response
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_http_message_t message;

    char* version;
    uint16_t status_code;
    char* reason_phrase;

} co_http_response_t;

CO_HTTP_API void co_http_response_serialize(
    const co_http_response_t* response, co_byte_array_t* data);
CO_HTTP_API int co_http_response_deserialize(
    co_http_response_t* response, const co_byte_array_t* data, size_t* index);

CO_HTTP_API void co_http_response_print_header(const co_http_response_t* response);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_response_t* co_http_response_create(void);
CO_HTTP_API co_http_response_t* co_http_response_create_with(
    uint16_t status_code, const char* reason_phrase);

CO_HTTP_API void co_http_response_destroy(co_http_response_t* response);

CO_HTTP_API co_http_header_t* co_http_response_get_header(co_http_response_t* response);
CO_HTTP_API const co_http_header_t* co_http_response_get_const_header(const co_http_response_t* response);

CO_HTTP_API bool co_http_response_set_content(
    co_http_response_t* response, const void* data, size_t data_size);

CO_HTTP_API const void* co_http_response_get_content(const co_http_response_t* response);
CO_HTTP_API size_t co_http_response_get_content_size(const co_http_response_t* response);

CO_HTTP_API void co_http_response_set_version(co_http_response_t* response, const char* version);
CO_HTTP_API const char* co_http_response_get_version(const co_http_response_t* response);

CO_HTTP_API void co_http_response_set_status_code(co_http_response_t* response, uint16_t status_code);
CO_HTTP_API uint16_t co_http_response_get_status_code(const co_http_response_t* response);

CO_HTTP_API void co_http_response_set_reason_phrase(co_http_response_t* response, const char* reason_phrase);
CO_HTTP_API const char* co_http_response_get_reason_phrase(const co_http_response_t* response);

CO_HTTP_API void co_http_response_add_cookie(
    co_http_response_t* response, co_http_cookie_st* cookie);
CO_HTTP_API size_t co_http_response_get_cookies(
    const co_http_response_t* response, co_http_cookie_st* cookies, size_t count);
CO_HTTP_API void co_http_response_remove_all_cookies(
    co_http_response_t* response);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_RESPONSE_H_INCLUDED
