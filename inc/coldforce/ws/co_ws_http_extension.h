#ifndef CO_WS_HTTP_EXTENSION_H_INCLUDED
#define CO_WS_HTTP_EXTENSION_H_INCLUDED

#include <coldforce/http/co_http_client.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_ws_client_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_WS_API struct co_ws_client_t* co_http_upgrade_to_ws(co_http_client_t* http_client);

CO_WS_API bool co_http_request_validate_ws_upgrade(
    const co_http_request_t* request);
CO_WS_API bool co_http_response_validate_ws_upgrade(
    const co_http_response_t* response,
    const co_ws_client_t* client, bool* upgrade_result);

CO_WS_API co_http_request_t* co_http_request_create_ws_upgrade(
    const char* path, const char* protocols, const char* extensions);
CO_WS_API co_http_response_t* co_http_response_create_ws_upgrade(
    const co_http_request_t* request, const char* protocol, const char* extensions);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_HTTP_EXTENSION_H_INCLUDED
