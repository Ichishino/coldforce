#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_content_receiver.h>

//---------------------------------------------------------------------------//
// http content receiver
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

int
co_http_receive_plain_data(
    co_http_content_receiver_t* receiver,
    co_byte_array_t* receive_data
)
{
    if (receiver->size > 0)
    {
        const char* data_ptr =
            (const char*)co_byte_array_get_ptr(receive_data, 0);
        const size_t data_size = co_byte_array_get_count(receive_data);

        size_t receive_size = data_size - receiver->index;
        size_t content_size = receiver->size - receiver->receive_size;

        if (receive_size < content_size)
        {
            content_size = receive_size;
        }

        if (content_size > 0)
        {
            if (receiver->fp == NULL)
            {
                co_byte_array_add(
                    receiver->data, &data_ptr[receiver->index], content_size);
            }
            else
            {
                fwrite(&data_ptr[receiver->index], 1, content_size, receiver->fp);
            }

            receiver->receive_size += content_size;
            receiver->index += content_size;
        }
    }

    if (receiver->size <= receiver->receive_size)
    {
        return CO_HTTP_PARSE_COMPLETE;
    }

    return CO_HTTP_PARSE_MORE_DATA;
}

int
co_http_receive_chunked_data(
    co_http_content_receiver_t* receiver,
    co_byte_array_t* receive_data
)
{
    const char* data_ptr =
        (const char*)co_byte_array_get_ptr(receive_data, 0);
    const size_t data_size = co_byte_array_get_count(receive_data);

    size_t receive_size = data_size - receiver->index;

    while (receive_size > 0)
    {
        if (receiver->chunk_size == 0)
        {
            const char* new_line = co_string_find_n(
                &data_ptr[receiver->index], CO_HTTP_CRLF, receive_size);

            if (new_line == NULL)
            {
                return CO_HTTP_PARSE_MORE_DATA;
            }

            if ((new_line - &data_ptr[receiver->index]) > CO_SIZE_T_HEX_DIGIT_MAX)
            {
                return CO_HTTP_PARSE_ERROR;
            }

            receiver->chunk_size =
                co_string_to_size_t(&data_ptr[receiver->index], NULL, 16);

            receiver->index +=
                (new_line - &data_ptr[receiver->index]) + CO_HTTP_CRLF_LENGTH;
            receive_size = data_size - receiver->index;
        }

        size_t content_size = 0;

        if (receiver->chunk_size > receiver->chunk_receive_size)
        {
            content_size = receiver->chunk_size - receiver->chunk_receive_size;
        }

        if (receive_size < content_size)
        {
            content_size = receive_size;
        }

        receiver->chunk_receive_size += content_size;

        if (receiver->chunk_size <= receiver->chunk_receive_size)
        {
            receiver->chunk_size = 0;
            receiver->chunk_receive_size = 0;
        }

        if (content_size > 0)
        {
            if (receiver->fp == NULL)
            {
                co_byte_array_add(
                    receiver->data, &data_ptr[receiver->index], content_size);
            }
            else
            {
                fwrite(&data_ptr[receiver->index], 1, content_size, receiver->fp);
            }

            receiver->receive_size += content_size;
            receiver->index += content_size;

            receive_size = data_size - receiver->index;
        }

        if (receiver->chunk_size == 0)
        {
            if (receive_size >= CO_HTTP_CRLF_LENGTH)
            {
                receiver->index += CO_HTTP_CRLF_LENGTH;
                receive_size = data_size - receiver->index;

                if (content_size == 0)
                {
                    return CO_HTTP_PARSE_COMPLETE;
                }
            }
        }
    }

    return CO_HTTP_PARSE_MORE_DATA;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_content_receiver_setup(
    co_http_content_receiver_t* receiver
)
{
    receiver->index = 0;
    receiver->chunked = false;
    receiver->chunk_size = 0;
    receiver->chunk_receive_size = 0;
    receiver->size = 0;
    receiver->receive_size = 0;
    receiver->data = co_byte_array_create();
    receiver->fp = NULL;
}

void
co_http_content_receiver_cleanup(
    co_http_content_receiver_t* receiver
)
{
    if (receiver != NULL)
    {
        co_http_content_receiver_clear(receiver);

        co_byte_array_destroy(receiver->data);
        receiver->data = NULL;
    }
}

void
co_http_content_receiver_clear(
    co_http_content_receiver_t* receiver
)
{
    receiver->index = 0;
    receiver->chunked = false;
    receiver->chunk_size = 0;
    receiver->chunk_receive_size = 0;
    receiver->size = 0;
    receiver->receive_size = 0;
    
    co_byte_array_set_count(receiver->data, 0);

    if (receiver->fp != NULL)
    {
        fclose(receiver->fp);
        receiver->fp = NULL;
    }
}

bool
co_http_start_receive_content(
    co_http_content_receiver_t* receiver,
    co_http_message_t* message,
    size_t index,
    const char* file_path
)
{
    const char* value = co_http_header_get_item(
        &message->header, CO_HTTP_HEADER_TRANSFER_ENCODING);

    if ((value != NULL) &&
        (co_string_case_compare(value, "chunked") == 0))
    {
        receiver->chunked = true;
    }
    else
    {
        co_http_header_get_content_length(
            &message->header, &receiver->size);
    }

    receiver->index = index;

    if (file_path != NULL)
    {
        receiver->fp = fopen(file_path, "wb");

        if (receiver->fp == NULL)
        {
            return false;
        }
    }

    return true;
}

int
co_http_receive_content_data(
    co_http_content_receiver_t* receiver,
    co_byte_array_t* receive_data
)
{
    if (receiver->chunked)
    {
        return co_http_receive_chunked_data(receiver, receive_data);
    }
    else
    {
        return co_http_receive_plain_data(receiver, receive_data);
    }
}

void
co_http_complete_receive_content(
    co_http_content_receiver_t* receiver,
    size_t* index,
    co_buffer_st* buffer
)
{
    *index = (size_t)receiver->index;

    buffer->size = co_byte_array_get_count(receiver->data);

    co_byte_array_add(receiver->data, "\0", 1);
    co_byte_array_set_count(receiver->data, buffer->size);

    if (buffer->size > 0)
    {
        buffer->ptr = co_byte_array_detach(receiver->data);
    }

    co_http_content_receiver_clear(receiver);
}

void
co_http_content_more_data(
    co_http_content_receiver_t* receiver,
    co_byte_array_t* receive_data
)
{
    receiver->index = 0;
    co_byte_array_set_count(receive_data, 0);
}
