#ifndef CO_HTTP_MESSAGE_H_INCLUDED
#define CO_HTTP_MESSAGE_H_INCLUDED

#include <coldforce/core/co_byte_array.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_header.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http message
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_http_header_t header;
    co_buffer_st content;

} co_http_message_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_http_message_setup(co_http_message_t* message);
void co_http_message_cleanup(co_http_message_t* message);

void co_http_message_serialize(
    const co_http_message_t* message, co_byte_array_t* buffer);
int co_http_message_deserialize_header(
    const co_byte_array_t* data, size_t* index, co_http_message_t* message);

bool co_http_message_set_content(
    co_http_message_t* request, const void* data, size_t data_size);

const void* co_http_message_get_content(const co_http_message_t* message);
size_t co_http_message_get_content_size(const co_http_message_t* message);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_MESSAGE_H_INCLUDED
