#ifndef CO_HTTP2_MESSAGE_H_INCLUDED
#define CO_HTTP2_MESSAGE_H_INCLUDED

#include <coldforce/core/co_list.h>

#include <coldforce/http2/co_http2.h>
#include <coldforce/http2/co_http2_header.h>


CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http2 message
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_http2_client_t;
struct co_http2_stream_t;
struct co_http2_message_t;

typedef void(*co_http2_message_fn)(
    void* self, struct co_http2_client_t* client, struct co_http2_stream_t* stream,
    const struct co_http2_message_t* receive_message, int error_code);

typedef struct co_http2_data_t
{
    uint8_t* ptr;
    size_t size;
    size_t index;

    char* file_path;

} co_http2_data_t;

typedef struct co_http2_message_t
{
    co_http2_header_t header;
    co_http2_data_t data;

} co_http2_message_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_HTTP2_API co_http2_message_t* co_http2_message_create(void);
CO_HTTP2_API co_http2_message_t* co_http2_message_create_request(const char* method, const char* path);
CO_HTTP2_API co_http2_message_t* co_http2_message_create_response(uint16_t status_code);

CO_HTTP2_API void co_http2_message_destroy(co_http2_message_t* message);

CO_HTTP2_API co_http2_header_t* co_http2_message_get_header(co_http2_message_t* message);
CO_HTTP2_API const co_http2_header_t* co_http2_message_get_const_header(const co_http2_message_t* message);

CO_HTTP2_API void co_http2_message_set_content(
    co_http2_message_t* message, const void* data, size_t data_size);
CO_HTTP2_API const void* co_http2_message_get_content(
    const co_http2_message_t* message);
CO_HTTP2_API size_t co_http2_message_get_content_size(
    const co_http2_message_t* message);

CO_HTTP2_API void co_http2_message_set_content_file_path(
    co_http2_message_t* message, const char* file_path);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP2_MESSAGE_H_INCLUDED
