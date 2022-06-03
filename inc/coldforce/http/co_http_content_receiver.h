#ifndef CO_HTTP_CONTENT_RECEIVER_H_INCLUDED
#define CO_HTTP_CONTENT_RECEIVER_H_INCLUDED

#include <stdio.h>

#include <coldforce/core/co_byte_array.h>

#include <coldforce/http/co_http.h>
#include <coldforce/http/co_http_message.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// http content receiver
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    size_t index;

    bool chunked;
    size_t chunk_size;
    size_t chunk_receive_size;

    size_t size;
    size_t receive_size;

    co_byte_array_t* data;
    FILE* fp;

} co_http_content_receiver_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void co_http_content_receiver_setup(co_http_content_receiver_t* receiver);
void co_http_content_receiver_cleanup(co_http_content_receiver_t* receiver);

void co_http_content_receiver_clear(
    co_http_content_receiver_t* receiver);

bool co_http_start_receive_content(
    co_http_content_receiver_t* receiver, co_http_message_t* message,
    size_t index, const char* file_path);
int co_http_receive_content_data(
    co_http_content_receiver_t* receiver, co_byte_array_t* receive_data);

void co_http_complete_receive_content(
    co_http_content_receiver_t* receiver, size_t* index, co_buffer_st* buffer);

void co_http_content_more_data(
    co_http_content_receiver_t* receiver, co_byte_array_t* receive_data);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_HTTP_CONTENT_RECEIVER_H_INCLUDED
