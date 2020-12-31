#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_message.h>

//---------------------------------------------------------------------------//
// http message
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_message_setup(
    co_http_message_t* message
)
{
    co_http_header_setup(&message->header);

    message->content.ptr = NULL;
    message->content.size = 0;
}

void
co_http_message_cleanup(
    co_http_message_t* message
)
{
    if (message != NULL)
    {
        co_http_header_cleanup(&message->header);

        co_mem_free(message->content.ptr);
    }
}

void
co_http_message_serialize(
    const co_http_message_t* message,
    co_byte_array_t* buffer
)
{
    co_http_header_serialize(&message->header, buffer);

    co_byte_array_add_string(buffer, CO_HTTP_CRLF);

    if ((message->content.ptr != NULL) &&
        (message->content.size > 0))
    {
        co_byte_array_add(
            buffer, message->content.ptr, message->content.size);
    }
}

int
co_http_message_deserialize_header(
    co_http_message_t* message,
    const co_byte_array_t* data,
    size_t* index
)
{
    return co_http_header_deserialize(&message->header, data, index);
}

bool
co_http_message_set_content(
    co_http_message_t* message,
    const void* data,
    size_t data_size
)
{
    co_mem_free(message->content.ptr);

    message->content.size = 0;
    message->content.ptr = NULL;

    if ((data != NULL) && (data_size > 0))
    {
        void* buffer = co_mem_alloc(data_size);

        if (buffer == NULL)
        {
            return false;
        }

        memcpy(buffer, data, data_size);

        message->content.size = data_size;
        message->content.ptr = buffer;
    }

    return true;
}

const void*
co_http_message_get_content(
    const co_http_message_t* message
)
{
    return message->content.ptr;
}

size_t
co_http_message_get_content_size(
    const co_http_message_t* message
)
{
    return message->content.size;
}
