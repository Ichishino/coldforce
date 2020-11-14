#include <coldforce/core/co_std.h>
#include <coldforce/core/co_array.h>

//---------------------------------------------------------------------------//
// array
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_array_t*
co_array_create(
    size_t element_length,
    co_array_ctx_t* ctx)
{
    co_array_t* arr =
        (co_array_t*)co_mem_alloc(sizeof(co_array_t));

    if (arr == NULL)
    {
        return NULL;
    }

    size_t capacity = 0;

    if (ctx != NULL)
    {
        capacity = ctx->initial_capacity;
    }

    if (capacity == 0)
    {
        capacity = CO_ARRAY_DEFAULT_CAPACITY;
    }

    arr->buffer =
        (uint8_t*)co_mem_alloc(element_length * capacity);

    if (arr->buffer == NULL)
    {
        co_mem_free(arr);

        return NULL;
    }

    arr->size = 0;
    arr->element_length = element_length;
    arr->capacity = capacity;

    return arr;
}

void
co_array_destroy(
    co_array_t* arr
)
{
    if (arr != NULL)
    {
        co_mem_free(arr->buffer);
        co_mem_free(arr);
    }
}

bool
co_array_set_size(
    co_array_t* arr,
    size_t size
)
{
    if (arr->capacity <= size)
    {
        size_t new_capacity = arr->capacity * 2;

        while (new_capacity <= size)
        {
            new_capacity *= 2;
        }

        void* new_buffer =
            co_mem_realloc(arr->buffer, arr->element_length * new_capacity);

        if (new_buffer == NULL)
        {
            return false;
        }

        arr->buffer = (uint8_t*)new_buffer;
    }

    arr->size = size;

    return true;
}

size_t
co_array_get_size(
    const co_array_t* arr
)
{
    return arr->size;
}

void*
co_array_get(
    co_array_t* arr,
    size_t index
)
{
    co_assert(index < arr->size);

    return &arr->buffer[arr->element_length * index];
}

void
co_array_zero_clear(
    co_array_t* arr
)
{
    memset(arr->buffer, 0x00,
        arr->element_length * arr->size);
}
