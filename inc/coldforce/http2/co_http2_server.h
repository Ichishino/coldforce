#ifndef CO_HTTP2_SERVER_H_INCLUDED
#define CO_HTTP2_SERVER_H_INCLUDED

#include <coldforce/http/co_http_server.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_http2_server_on_tcp_receive_ready(
    co_thread_t* thread, co_tcp_client_t* tcp_client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API co_http2_client_t* co_http2_client_create_with(
    co_tcp_client_t* tcp_client);

CO_HTTP2_API bool co_http2_send_ping(
    co_http2_client_t* client, uint64_t user_data, co_http2_ping_fn handler);

CO_HTTP2_API void co_http2_set_priority_handler(
    co_http2_client_t* client, co_http2_priority_fn handler);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_SERVER_H_INCLUDED
