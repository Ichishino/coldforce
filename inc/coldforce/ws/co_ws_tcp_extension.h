#ifndef CO_WS_TCP_EXTENSION_H_INCLUDED
#define CO_WS_TCP_EXTENSION_H_INCLUDED

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

struct co_ws_client_t;

//---------------------------------------------------------------------------//
// tcp extension for websocket
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_API
struct co_ws_client_t*
co_tcp_upgrade_to_ws(
    co_tcp_client_t* tcp_client,
    const char* url_origin
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_TCP_EXTENSION_H_INCLUDED
