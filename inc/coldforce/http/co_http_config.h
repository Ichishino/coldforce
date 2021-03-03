#ifndef CO_HTTP_CONFIG_H_INCLUDED
#define CO_HTTP_CONFIG_H_INCLUDED

#include <coldforce/http/co_http.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http config
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP_CONFIG_MAX_RECEIVE_HEADER_LINE_SIZE       8192
#define CO_HTTP_CONFIG_MAX_RECEIVE_HEADER_FIELD_COUNT     1024
#define CO_HTTP_CONFIG_MAX_RECEIVE_CONTENT_SIZE         SIZE_MAX

typedef struct
{
    size_t max_receive_header_line_size;
    size_t max_receive_header_field_count;
    size_t max_receive_content_size;

} co_http_config_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API void co_http_config_set_max_receive_header_line_size(
    size_t max_receive_header_line_size);
CO_HTTP_API size_t co_http_config_get_max_receive_header_line_size(void);

CO_HTTP_API void co_http_config_set_max_receive_header_field_count(
    size_t max_receive_header_field_count);
CO_HTTP_API size_t co_http_config_get_max_receive_header_field_count(void);

CO_HTTP_API void co_http_config_set_max_receive_content_size(
    size_t max_receive_content_size);
CO_HTTP_API size_t co_http_config_get_max_receive_content_size(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_CONFIG_H_INCLUDED
