#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_message.h>

//---------------------------------------------------------------------------//
// http message
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_http_message_setup(
    co_http_message_t* message
)
{
    co_http_header_setup(&message->header);

    message->data.ptr = NULL;
    message->data.size = 0;
}

void
co_http_message_cleanup(
    co_http_message_t* message
)
{
    if (message != NULL)
    {
        co_http_header_cleanup(&message->header);

        co_mem_free(message->data.ptr);
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

    if ((message->data.ptr != NULL) &&
        (message->data.size > 0))
    {
        co_byte_array_add(
            buffer, message->data.ptr, message->data.size);
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
co_http_message_set_data(
    co_http_message_t* message,
    const void* data,
    size_t data_size
)
{
    co_mem_free(message->data.ptr);

    message->data.size = 0;
    message->data.ptr = NULL;

    if ((data != NULL) && (data_size > 0))
    {
        void* buffer = co_mem_alloc(data_size);

        if (buffer == NULL)
        {
            return false;
        }

        memcpy(buffer, data, data_size);

        message->data.size = data_size;
        message->data.ptr = buffer;
    }

    return true;
}

const void*
co_http_message_get_data(
    const co_http_message_t* message
)
{
    return message->data.ptr;
}

size_t
co_http_message_get_data_size(
    const co_http_message_t* message
)
{
    return message->data.size;
}
