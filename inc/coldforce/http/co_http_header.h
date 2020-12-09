#ifndef CO_HTTP_HEADER_H_INCLUDED
#define CO_HTTP_HEADER_H_INCLUDED

#include <coldforce/core/co_list.h>
#include <coldforce/core/co_byte_array.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_cookie.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http header
//---------------------------------------------------------------------------//

#define CO_HTTP_HEADER_CONTENT_LENGTH               "Content-Length"
#define CO_HTTP_HEADER_HOST                         "Host"
#define CO_HTTP_HEADER_TRANSFER_ENCODING            "Transfer-Encoding"
#define CO_HTTP_HEADER_SET_COOKIE                   "Set-Cookie"
#define CO_HTTP_HEADER_COOKIE                       "Cookie"

typedef struct
{
    char* name;
    char* value;

} co_http_header_item_t;

typedef struct
{
    co_list_t* items;

} co_http_header_t;

void co_http_header_setup(co_http_header_t* header);
void co_http_header_cleanup(co_http_header_t* header);

void co_http_header_serialize(
    const co_http_header_t* header, co_byte_array_t* buffer);
int co_http_header_deserialize(
    const co_byte_array_t* data, size_t* index, co_http_header_t* header);

void co_http_header_print(const co_http_header_t* header, FILE* fp);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API void co_http_header_clear(co_http_header_t* header);

CO_HTTP_API size_t co_http_header_get_item_count(const co_http_header_t* header);

CO_HTTP_API size_t co_http_header_get_value_count(const co_http_header_t* header, const char* name);

CO_HTTP_API bool co_http_header_contains(const co_http_header_t* header, const char* name);

CO_HTTP_API void co_http_header_set_item(co_http_header_t* header, const char* key, const char* value);

CO_HTTP_API const char* co_http_header_get_item(const co_http_header_t* header, const char* name);

CO_HTTP_API size_t co_http_header_get_items(
    const co_http_header_t* header, const char* name, const char* value[], size_t count);

CO_HTTP_API bool co_http_header_add_item(
    co_http_header_t* header, const char* name, const char* value);

CO_HTTP_API void co_http_header_remove_item(co_http_header_t* header, const char* name);
CO_HTTP_API void co_http_header_remove_all_values(co_http_header_t* header, const char* name);

CO_HTTP_API void co_http_header_set_content_length(co_http_header_t* header, size_t length);
CO_HTTP_API bool co_http_header_get_content_length(const co_http_header_t* header, size_t* length);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_HEADER_H_INCLUDED
