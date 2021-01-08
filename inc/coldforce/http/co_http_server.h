#ifndef CO_HTTP_SERVER_H_INCLUDED
#define CO_HTTP_SERVER_H_INCLUDED

#include <coldforce/tls/co_tls_tcp_server.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_client.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http_server_t;

typedef bool(*co_http_upgrade_request_fn)(
    void* self, struct co_http_client_t* client);

typedef struct
{
    void (*destroy)(co_tcp_server_t*);
    void (*close)(co_tcp_server_t*);
    bool (*start)(co_tcp_server_t*, co_tcp_accept_fn, int);

} co_tcp_server_module_t;

typedef struct co_http_server_t
{
    co_tcp_server_t* tcp_server;
    co_tcp_server_module_t module;

} co_http_server_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_server_t* co_http_server_create(
    const co_net_addr_t* local_net_addr);

CO_HTTP_API co_http_server_t* co_http_tls_server_create(
    const co_net_addr_t* local_net_addr, co_tls_ctx_st* tls_ctx);

CO_HTTP_API void co_http_server_destroy(co_http_server_t* server);
CO_HTTP_API void co_http_server_close(co_http_server_t* server);

CO_HTTP_API bool co_http_server_start(
    co_http_server_t* server, co_tcp_accept_fn handler, int backlog);

CO_HTTP_API co_socket_t* co_http_server_get_socket(co_http_server_t* server);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_client_t* co_http_client_create_with(
    co_tcp_client_t* tcp_client);

CO_HTTP_API void co_http_set_request_handler(
    co_http_client_t* client, co_http_request_fn handler);

CO_HTTP_API bool co_http_send_response(
    co_http_client_t* client, co_http_response_t* response);

CO_HTTP_API bool co_http_begin_chunked_response(
    co_http_client_t* client, co_http_response_t* response);
CO_HTTP_API bool co_http_send_chunked_response(
    co_http_client_t* client, const void* data, size_t data_length);
CO_HTTP_API bool co_http_end_chunked_response(
    co_http_client_t* client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_client_t* co_tcp_get_http_client(co_tcp_client_t* tcp_client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_SERVER_H_INCLUDED
