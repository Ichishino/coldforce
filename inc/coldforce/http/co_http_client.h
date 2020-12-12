#ifndef CO_HTTP_CLIENT_H_INCLUDED
#define CO_HTTP_CLIENT_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/net/co_tcp_client.h>

#include <coldforce/tls/co_tls.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_url.h>
#include <coldforce/http/co_http_request.h>
#include <coldforce/http/co_http_response.h>
#include <coldforce/http/co_http_content_receiver.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http client
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_HTTP_ERROR_CONNECT_FAILED        -9001
#define CO_HTTP_ERROR_TLS_HANDSHAKE_FAILED  -9002
#define CO_HTTP_ERROR_CONNECTION_CLOSED     -9003
#define CO_HTTP_ERROR_OUT_OF_MEMORY         -9004
#define CO_HTTP_ERROR_PARSE_HEADER          -9005
#define CO_HTTP_ERROR_PARSE_CONTENT         -9006
#define CO_HTTP_ERROR_CANCEL                -9007

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http_client_t;

typedef void(*co_http_tls_handshake_fn)(
    void* self, struct co_http_client_t* client, int error_code);

typedef void(*co_http_request_fn)(
    void* self, struct co_http_client_t* client,
    const co_http_request_t* request,
    int error_code);

typedef void(*co_http_response_fn)(
    void* self, struct co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* response,
    int error_code);

typedef bool(*co_http_progress_fn)(
    void* self, struct co_http_client_t* client,
    const co_http_request_t* request, const co_http_response_t* response,
    size_t current_content_size);

typedef void(*co_http_close_fn)(
    void* self, struct co_http_client_t* client);

typedef struct
{
    void (*destroy)(co_tcp_client_t*);
    void (*close)(co_tcp_client_t*);
    bool (*connect_async)(co_tcp_client_t*,
        const co_net_addr_t*, co_tcp_connect_fn);
    bool (*send)(co_tcp_client_t*, const void*, size_t);
    ssize_t (*receive_all)(co_tcp_client_t*, co_byte_array_t*);

} co_tcp_client_module_t;

typedef struct co_http_client_t
{
    co_tcp_client_t* tcp_client;
    co_tcp_client_module_t module;

    bool connecting;
    co_http_url_st* base_url;
    co_list_t* send_queue;
    co_list_t* receive_queue;

    co_byte_array_t* receive_data;
    co_http_response_t* response;
    co_http_content_receiver_t content_receiver;

    co_http_response_fn on_response;
    co_http_progress_fn on_progress;
    co_http_close_fn on_close;

    co_http_request_t* request;
    co_http_request_fn on_request;
    co_http_tls_handshake_fn on_handshake_complete;

} co_http_client_t;

void co_http_client_setup(co_http_client_t* client, bool secure);

void co_http_client_on_connect(
    co_thread_t* thread, co_tcp_client_t* tcp_client, int error_code);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP_API co_http_client_t* co_http_client_create(
    const char* base_url, const co_net_addr_t* local_net_addr, co_tls_ctx_st* tls_ctx);

CO_HTTP_API void co_http_client_destroy(co_http_client_t* client);

CO_HTTP_API bool co_http_request_async(
    co_http_client_t* client, co_http_request_t* request);

CO_HTTP_API bool co_http_send_data(
    co_http_client_t* client, const void* data, size_t data_size);

CO_HTTP_API void co_http_set_response_handler(
    co_http_client_t* client, co_http_response_fn handler);
CO_HTTP_API void co_http_set_progress_handler(
    co_http_client_t* client, co_http_progress_fn handler);
CO_HTTP_API void co_http_set_close_handler(
    co_http_client_t* client, co_http_close_fn handler);

CO_HTTP_API const co_net_addr_t* co_http_get_remote_net_addr(const co_http_client_t* client);

CO_HTTP_API co_socket_t* co_http_client_get_socket(co_http_client_t* client);
CO_HTTP_API const char* co_http_get_base_url(const co_http_client_t* client);
CO_HTTP_API bool co_http_is_open(const co_http_client_t* client);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_CLIENT_H_INCLUDED
