#ifndef CO_WS_HTTP_EXTENSION_H_INCLUDED
#define CO_WS_HTTP_EXTENSION_H_INCLUDED

#include <coldforce/http/co_http_client.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

struct co_ws_client_t;

//---------------------------------------------------------------------------//
// http extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_API
struct co_ws_client_t*
co_http_upgrade_to_ws(
    co_http_client_t* http_client
);

CO_WS_API
bool
co_http_request_validate_ws_upgrade(
    const co_http_request_t* request
);

CO_WS_API
bool
co_http_response_validate_ws_upgrade(
    const co_http_response_t* response,
    const co_http_request_t* request
);

CO_WS_API
co_http_request_t*
co_http_request_create_ws_upgrade(
    const char* path,
    const char* protocols,
    const char* extensions
);

CO_WS_API
co_http_response_t*
co_http_response_create_ws_upgrade(
    const co_http_request_t* request,
    const char* protocol,
    const char* extensions
);

CO_WS_API
bool
co_http_connection_send_ws_frame(
    co_http_connection_t* conn,
    bool fin,
    uint8_t opcode,
    bool mask,
    const void* data,
    size_t data_size
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_HTTP_EXTENSION_H_INCLUDED
