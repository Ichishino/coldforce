#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_header.h>

#ifndef CO_OS_WIN
#include <errno.h>
#endif

//---------------------------------------------------------------------------//
// http header
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_header_item_destroy(
    co_http_header_item_t* item
)
{
    co_mem_free(item->name);
    co_mem_free(item->value);
    co_mem_free(item);
}

intptr_t
co_http_header_item_compare(
    const co_http_header_item_t* item1,
    const co_http_header_item_t* item2
)
{
    return co_string_case_compare(item1->name, item2->name);
}

bool
co_http_header_add_raw_item(
    co_http_header_t* header,
    char* name,
    char* value
)
{
    co_http_header_item_t* item =
        (co_http_header_item_t*)co_mem_alloc(
            sizeof(co_http_header_item_t));

    if (item == NULL)
    {
        return false;
    }

    item->name = name;
    item->value = value;

    return co_list_add_tail(header->items, (uintptr_t)item);
}

void
co_http_header_serialize(
    const co_http_header_t* header,
    co_byte_array_t* buffer
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->items);

    while (it != NULL)
    {
        const co_list_data_st* list_data =
            co_list_get_next(header->items, &it);
        const co_http_header_item_t* item =
            (const co_http_header_item_t*)list_data->value;

        co_byte_array_add_string(buffer, item->name);
        co_byte_array_add_string(buffer, CO_HTTP_COLON);
        co_byte_array_add_string(buffer, item->value);
        co_byte_array_add_string(buffer, CO_HTTP_CRLF);
    }
}

int
co_http_header_deserialize(
    const co_byte_array_t* data,
    size_t* index,
    co_http_header_t* header
)
{
    const char* data_ptr =
        (const char*)co_byte_array_get_ptr((co_byte_array_t*)data, 0);
    const size_t data_size = co_byte_array_get_count(data);

    for (;;)
    {
        const char* temp_data = &data_ptr[(*index)];
        size_t temp_size = data_size - (*index);

        const char* new_line =
            co_string_find_n(temp_data, CO_HTTP_CRLF, temp_size);

        if (new_line == NULL)
        {
            return CO_HTTP_PARSE_MORE_DATA;
        }

        if (new_line == temp_data)
        {
            (*index) += CO_HTTP_CRLF_LENGTH;

            const char* value =
                co_http_header_get_item(header, CO_HTTP_HEADER_CONTENT_LENGTH);
            
            if (value != NULL)
            {
                size_t length = strlen(value);

                while ((*value) != '\0')
                {
                    if (((*value) < '0') || ((*value) > '9'))
                    {
                        return CO_HTTP_PARSE_ERROR;
                    }

                    ++value;
                }

                if (length > CO_SIZE_T_DEC_DIGIT_MAX)
                {
                    return CO_HTTP_PARSE_ERROR;
                }
                else if (length == CO_SIZE_T_DEC_DIGIT_MAX)
                {
                    (void)co_string_to_size_t(value, NULL, 10);

                    if (errno == ERANGE)
                    {
                        return CO_HTTP_PARSE_ERROR;
                    }
                }
            }

            return CO_HTTP_PARSE_COMPLETE;
        }

        const char* colon =
            co_string_find_n(temp_data, CO_HTTP_COLON, (new_line - temp_data));

        if (colon == NULL)
        {
            return CO_HTTP_PARSE_MORE_DATA;
        }

        size_t name_size = colon - temp_data;
        size_t value_size = new_line - colon - CO_HTTP_COLON_LENGTH;

        char* name = co_string_duplicate_n(temp_data, name_size);

        if (name == NULL)
        {
            return CO_HTTP_PARSE_ERROR;
        }

        char* value = co_string_duplicate_n(
            temp_data + name_size + CO_HTTP_COLON_LENGTH, value_size);

        if (value == NULL)
        {
            co_mem_free(name);

            return CO_HTTP_PARSE_ERROR;
        }

        co_string_trim(name, strlen(name));
        co_string_trim(value, strlen(value));

        co_http_header_add_raw_item(header, name, value);

        (*index) += (new_line - temp_data) + CO_HTTP_CRLF_LENGTH;
    }
}

void
co_http_header_print(
    const co_http_header_t* header,
    FILE* fp
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->items);

    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_next(header->items, &it);
        const co_http_header_item_t* item =
            (const co_http_header_item_t*)data->value;

        fprintf(fp, "%s: %s\n", item->name, item->value);
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_header_setup(
    co_http_header_t* header
)
{
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_free_fn)co_http_header_item_destroy;
    list_ctx.compare_values = (co_compare_fn)co_http_header_item_compare;

    header->items = co_list_create(&list_ctx);
}

void
co_http_header_cleanup(
    co_http_header_t* header
)
{
    if (header != NULL)
    {
        co_list_destroy(header->items);
        header->items = NULL;
    }
}

void
co_http_header_clear(
    co_http_header_t* header
)
{
    co_list_clear(header->items);
}

size_t
co_http_header_get_item_count(
    const co_http_header_t* header
)
{
    return co_list_get_count(header->items);
}

size_t
co_http_header_get_value_count(
    const co_http_header_t* header,
    const char* name
)
{
    size_t value_count = 0;

    co_list_iterator_t* it =
        co_list_get_head_iterator(header->items);
    
    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_next(header->items, &it);

        if (co_string_case_compare(
            ((co_http_header_item_t*)data->value)->name, name) == 0)
        {
            ++value_count;
        }
    }

    return value_count;
}

bool
co_http_header_contains(
    const co_http_header_t* header,
    const char* name
)
{
    const co_http_header_item_t find_item = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->items, (uintptr_t)&find_item);

    return (it != NULL);
}

void
co_http_header_set_item(
    co_http_header_t* header,
    const char* name,
    const char* value
)
{
    const co_http_header_item_t find_item = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->items, (uintptr_t)&find_item);

    if (it != NULL)
    {
        co_http_header_item_t* item =
            (co_http_header_item_t*)it->data.value;

        co_mem_free(item->value);
        item->value = co_string_duplicate(value);
    }
    else
    {
        co_http_header_add_item(header, name, value);
    }
}

const char*
co_http_header_get_item(
    const co_http_header_t* header,
    const char* name
)
{
    const co_http_header_item_t find_item = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->items, (uintptr_t)&find_item);

    if (it != NULL)
    {
        const co_http_header_item_t* item =
            (const co_http_header_item_t*)co_list_get(
                header->items, it)->value;

        return item->value;
    }

    return NULL;
}

size_t
co_http_header_get_items(
    const co_http_header_t* header,
    const char* name,
    const char* value[],
    size_t count
)
{
    size_t value_count = 0;

    co_list_iterator_t* it =
        co_list_get_head_iterator(header->items);

    while (it != NULL && (value_count < count))
    {
        const co_list_data_st* data =
            co_list_get_next(header->items, &it);

        if (co_string_case_compare(
            ((const co_http_header_item_t*)data->value)->name, name) == 0)
        {
            value[value_count] =
                ((const co_http_header_item_t*)data->value)->value;

            ++value_count;
        }
    }

    return value_count;
}

bool
co_http_header_add_item(
    co_http_header_t* header,
    const char* name,
    const char* value
)
{
    co_http_header_item_t* item =
        (co_http_header_item_t*)co_mem_alloc(sizeof(co_http_header_item_t));

    if (item == NULL)
    {
        return false;
    }

    item->name = co_string_duplicate(name);

    if (item->name == NULL)
    {
        co_mem_free(item);

        return false;
    }

    item->value = co_string_duplicate(value);

    if (item->value == NULL)
    {
        co_mem_free(item->name);
        co_mem_free(item);

        return false;
    }

    return co_list_add_tail(header->items, (uintptr_t)item);
}

void
co_http_header_remove_item(
    co_http_header_t* header,
    const char* name
)
{
    co_http_header_item_t find_item = { (char*)name, NULL };

    co_list_remove(header->items, (uintptr_t)&find_item);
}

void
co_http_header_remove_all_values(
    co_http_header_t* header,
    const char* name
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->items);

    while (it != NULL)
    {
        const co_list_data_st* data = co_list_get(header->items, it);

        if (co_string_case_compare(
            ((co_http_header_item_t*)data->value)->name, name) == 0)
        {
            co_list_iterator_t* temp = it;
            it = co_list_get_next_iterator(header->items, it);

            co_list_remove_at(header->items, temp);
        }
        else
        {
            it = co_list_get_next_iterator(header->items, it);
        }
    }
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http_header_set_content_length(
    co_http_header_t* header,
    size_t length
)
{
    char str[32];
    sprintf(str, "%zu", length);

    co_http_header_set_item(header, CO_HTTP_HEADER_CONTENT_LENGTH, str);
}

bool
co_http_header_get_content_length(
    const co_http_header_t* header,
    size_t* length
)
{
    const char* str =
        co_http_header_get_item(header, CO_HTTP_HEADER_CONTENT_LENGTH);

    if (str == NULL)
    {
        return false;
    }

    *length = co_string_to_size_t(str, NULL, 10);

    return true;
}
