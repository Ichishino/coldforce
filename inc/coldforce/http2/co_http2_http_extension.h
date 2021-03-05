#ifndef CO_HTTP2_HTTP_EXTENSION_H_INCLUDED
#define CO_HTTP2_HTTP_EXTENSION_H_INCLUDED

#include <coldforce/http/co_http_client.h>

#include <coldforce/http2/co_http2.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http extension for http2
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http2_client_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API struct co_http2_client_t* co_http_upgrade_to_http2(co_http_client_t* http_client);

CO_HTTP2_API bool co_http_request_is_connection_preface(const co_http_request_t* request);

CO_HTTP2_API bool co_http_request_is_http2_upgrade(const co_http_request_t* request);

CO_HTTP2_API co_http_request_t* co_http_request_create_http2_upgrade(
    const struct co_http2_client_t* client, const char* path,
    const co_http2_setting_param_st* param, uint16_t param_count);
CO_HTTP2_API co_http_response_t* co_http_response_create_http2_upgrade(
    uint16_t status_code, const char* reason_phrase);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_HTTP_EXTENSION_H_INCLUDED
