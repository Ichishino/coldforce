#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http2/co_http2_message.h>

//---------------------------------------------------------------------------//
// http2 message
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_http2_message_t*
co_http2_message_create(
    void
)
{
    co_http2_message_t* message =
        (co_http2_message_t*)co_mem_alloc(sizeof(co_http2_message_t));

    if (message == NULL)
    {
        return NULL;
    }

    co_http2_header_setup(&message->header);

    memset(&message->data,
        0x00, sizeof(co_http2_data_t));

    return message;
}

co_http2_message_t*
co_http2_message_create_request(
    const char* method,
    const char* path
)
{
    co_http2_message_t* message = co_http2_message_create();

    if (message == NULL)
    {
        return NULL;
    }

    co_http2_header_set_method(&message->header, method);
    co_http2_header_set_path(&message->header, path);

    return message;
}

void
co_http2_message_destroy(
    co_http2_message_t* message
)
{
    if (message != NULL)
    {
        co_http2_header_cleanup(&message->header);

        co_mem_free(message->data.ptr);
        co_string_destroy(message->data.file_path);

        co_mem_free(message);
    }
}

co_http2_header_t*
co_http2_message_get_header(
    co_http2_message_t* message
)
{
    return &message->header;
}

const co_http2_header_t*
co_http2_message_get_const_header(
    const co_http2_message_t* message
)
{
    return &message->header;
}

void
co_http2_message_set_content(
    co_http2_message_t* message,
    const void* data,
    size_t data_size
)
{
    co_mem_free(message->data.ptr);
    message->data.ptr = NULL;
    message->data.size = data_size;

    if (message->data.size > 0)
    {
        message->data.ptr = co_mem_alloc(data_size);

        memcpy(message->data.ptr,
            data, message->data.size);
    }
}

const void*
co_http2_message_get_content(
    const co_http2_message_t* message
)
{
    return message->data.ptr;
}

size_t
co_http2_message_get_content_size(
    const co_http2_message_t* message
)
{
    return message->data.size;
}

void
co_http2_message_set_content_file_path(
    co_http2_message_t* message,
    const char* file_path
)
{
    co_string_destroy(message->data.file_path);

    if (file_path != NULL)
    {
        message->data.file_path = co_string_duplicate(file_path);
    }
    else
    {
        message->data.file_path = NULL;
    }
}
