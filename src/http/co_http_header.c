#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/http/co_http_header.h>
#include <coldforce/http/co_http_config.h>

#ifndef CO_OS_WIN
#include <errno.h>
#endif

//---------------------------------------------------------------------------//
// http header
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static void
co_http_header_field_destroy(
    co_http_header_field_t* field
)
{
    co_string_destroy(field->name);
    co_string_destroy(field->value);
    co_mem_free(field);
}

static int
co_http_header_field_compare(
    const co_http_header_field_t* field1,
    const co_http_header_field_t* field2
)
{
    return co_string_case_compare(field1->name, field2->name);
}

bool
co_http_header_add_field_ptr(
    co_http_header_t* header,
    char* name,
    char* value
)
{
    co_http_header_field_t* field =
        (co_http_header_field_t*)co_mem_alloc(
            sizeof(co_http_header_field_t));

    if (field == NULL)
    {
        return false;
    }

    field->name = name;
    field->value = value;

    return co_list_add_tail(header->field_list, field);
}

void
co_http_header_serialize(
    const co_http_header_t* header,
    co_byte_array_t* buffer
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* list_data =
            co_list_get_next(header->field_list, &it);
        const co_http_header_field_t* field =
            (const co_http_header_field_t*)list_data->value;

        co_byte_array_add_string(buffer, field->name);
        co_byte_array_add_string(buffer, CO_HTTP_COLON);
        co_byte_array_add_string(buffer, field->value);
        co_byte_array_add_string(buffer, CO_HTTP_CRLF);
    }
}

int
co_http_header_deserialize(
    co_http_header_t* header,
    const co_byte_array_t* data,
    size_t* index
)
{
    const char* data_ptr =
        (const char*)co_byte_array_get_const_ptr(data, 0);
    const size_t data_size = co_byte_array_get_count(data);

    const size_t max_header_line_size =
        co_http_config_get_max_receive_header_line_size();
    const size_t max_header_field_count =
        co_http_config_get_max_receive_header_field_count();

    for (;;)
    {
        if (co_http_header_get_field_count(header) >
            max_header_field_count)
        {
            return CO_HTTP_ERROR_HEADER_FIELDS_TOO_MANY;
        }

        const char* temp_data = &data_ptr[(*index)];
        size_t temp_size = data_size - (*index);

        const char* new_line =
            co_string_find_n(temp_data, CO_HTTP_CRLF, temp_size);

        if (new_line == NULL)
        {
            if (temp_size > max_header_line_size)
            {
                return CO_HTTP_ERROR_HEADER_LINE_TOO_LONG;
            }
            else
            {
                return CO_HTTP_PARSE_MORE_DATA;
            }
        }

        if (new_line == temp_data)
        {
            (*index) += CO_HTTP_CRLF_LENGTH;

            const char* value =
                co_http_header_get_field(header, CO_HTTP_HEADER_CONTENT_LENGTH);
            
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

        size_t line_length = (new_line - temp_data);

        if (line_length > max_header_line_size)
        {
            return CO_HTTP_ERROR_HEADER_LINE_TOO_LONG;
        }

        const char* colon =
            co_string_find_n(temp_data, CO_HTTP_COLON, line_length);

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
            co_string_destroy(name);

            return CO_HTTP_PARSE_ERROR;
        }

        co_string_trim(name, strlen(name));
        co_string_trim(value, strlen(value));

        co_http_header_add_field_ptr(header, name, value);

        (*index) += line_length + CO_HTTP_CRLF_LENGTH;
    }
}

void
co_http_header_setup(
    co_http_header_t* header
)
{
    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value =
        (co_item_destroy_fn)co_http_header_field_destroy;
    list_ctx.compare_values =
        (co_item_compare_fn)co_http_header_field_compare;

    header->field_list = co_list_create(&list_ctx);
}

void
co_http_header_cleanup(
    co_http_header_t* header
)
{
    if (header != NULL)
    {
        co_list_destroy(header->field_list);
        header->field_list = NULL;
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_http_header_clear(
    co_http_header_t* header
)
{
    co_list_clear(header->field_list);
}

size_t
co_http_header_get_field_count(
    const co_http_header_t* header
)
{
    return co_list_get_count(header->field_list);
}

size_t
co_http_header_get_value_count(
    const co_http_header_t* header,
    const char* name
)
{
    size_t value_count = 0;

    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);
    
    while (it != NULL)
    {
        const co_list_data_st* data =
            co_list_get_next(header->field_list, &it);

        if (co_string_case_compare(
            ((co_http_header_field_t*)data->value)->name, name) == 0)
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
    const co_http_header_field_t find_field = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->field_list, &find_field);

    return (it != NULL);
}

void
co_http_header_set_field(
    co_http_header_t* header,
    const char* name,
    const char* value
)
{
    const co_http_header_field_t find_field = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->field_list, &find_field);

    if (it != NULL)
    {
        co_http_header_field_t* field =
            (co_http_header_field_t*)it->data.value;

        co_string_destroy(field->value);
        field->value = co_string_duplicate(value);
    }
    else
    {
        co_http_header_add_field(header, name, value);
    }
}

const char*
co_http_header_get_field(
    const co_http_header_t* header,
    const char* name
)
{
    const co_http_header_field_t find_field = { (char*)name, NULL };

    co_list_iterator_t* it =
        co_list_find(header->field_list, &find_field);

    if (it != NULL)
    {
        const co_http_header_field_t* field =
            (const co_http_header_field_t*)co_list_get(
                header->field_list, it)->value;

        return field->value;
    }

    return NULL;
}

size_t
co_http_header_get_fields(
    const co_http_header_t* header,
    const char* name,
    const char* value[],
    size_t count
)
{
    size_t value_count = 0;

    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);

    while (it != NULL && (value_count < count))
    {
        const co_list_data_st* data =
            co_list_get_next(header->field_list, &it);

        if (co_string_case_compare(
            ((const co_http_header_field_t*)data->value)->name, name) == 0)
        {
            value[value_count] =
                ((const co_http_header_field_t*)data->value)->value;

            ++value_count;
        }
    }

    return value_count;
}

bool
co_http_header_add_field(
    co_http_header_t* header,
    const char* name,
    const char* value
)
{
    co_http_header_field_t* field =
        (co_http_header_field_t*)co_mem_alloc(sizeof(co_http_header_field_t));

    if (field == NULL)
    {
        return false;
    }

    field->name = co_string_duplicate(name);

    if (field->name == NULL)
    {
        co_mem_free(field);

        return false;
    }

    field->value = co_string_duplicate(value);

    if (field->value == NULL)
    {
        co_string_destroy(field->name);
        co_mem_free(field);

        return false;
    }

    return co_list_add_tail(header->field_list, field);
}

void
co_http_header_remove_field(
    co_http_header_t* header,
    const char* name
)
{
    co_http_header_field_t find_field = { (char*)name, NULL };

    co_list_remove(header->field_list, &find_field);
}

void
co_http_header_remove_all_fields(
    co_http_header_t* header,
    const char* name
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(header->field_list);

    while (it != NULL)
    {
        const co_list_data_st* data = co_list_get(header->field_list, it);

        if (co_string_case_compare(
            ((co_http_header_field_t*)data->value)->name, name) == 0)
        {
            co_list_iterator_t* temp = it;
            it = co_list_get_next_iterator(header->field_list, it);

            co_list_remove_at(header->field_list, temp);
        }
        else
        {
            it = co_list_get_next_iterator(header->field_list, it);
        }
    }
}

void
co_http_header_set_content_length(
    co_http_header_t* header,
    size_t length
)
{
    char str[32];
    sprintf(str, "%zu", length);

    co_http_header_set_field(header, CO_HTTP_HEADER_CONTENT_LENGTH, str);
}

bool
co_http_header_get_content_length(
    const co_http_header_t* header,
    size_t* length
)
{
    const char* str =
        co_http_header_get_field(header, CO_HTTP_HEADER_CONTENT_LENGTH);

    if (str == NULL)
    {
        return false;
    }

    *length = co_string_to_size_t(str, NULL, 10);

    if (errno != 0)
    {
        return false;
    }

    return true;
}
