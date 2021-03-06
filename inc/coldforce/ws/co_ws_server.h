#ifndef CO_WS_SERVER_H_INCLUDED
#define CO_WS_SERVER_H_INCLUDED

#include <coldforce/http/co_http_client.h>

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_ws_client_t;

void co_ws_server_on_receive_ready(
    co_thread_t* thread, co_tcp_client_t* tcp_client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_WS_API struct co_ws_client_t* co_ws_client_create_with(co_tcp_client_t* tcp_client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_SERVER_H_INCLUDED
