#include <coldforce/core/co_std.h>
#include <coldforce/core/co_array.h>

//---------------------------------------------------------------------------//
// array
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_array_t*
co_array_create(
    size_t element_size)
{
    co_array_t* arr =
        (co_array_t*)co_mem_alloc(sizeof(co_array_t));

    if (arr == NULL)
    {
        return NULL;
    }

    arr->capacity = 8;
    arr->buffer =
        (uint8_t*)co_mem_alloc(element_size * arr->capacity);

    if (arr->buffer == NULL)
    {
        co_mem_free(arr);

        return NULL;
    }

    arr->count = 0;
    arr->element_size = element_size;

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

void*
co_array_detach(
    co_array_t* arr
)
{
    void* ptr = arr->buffer;

    arr->capacity = 8;
    arr->buffer =
        (uint8_t*)co_mem_alloc(arr->element_size * arr->capacity);
    arr->count = 0;

    return ptr;
}

bool
co_array_set_count(
    co_array_t* arr,
    size_t count
)
{
    if (arr->capacity <= count)
    {
        size_t new_capacity = arr->capacity * 2;

        while (new_capacity <= count)
        {
            new_capacity *= 2;
        }

        void* new_buffer =
            co_mem_realloc(arr->buffer, arr->element_size * new_capacity);

        if (new_buffer == NULL)
        {
            return false;
        }

        arr->capacity = new_capacity;
        arr->buffer = (uint8_t*)new_buffer;
    }

    arr->count = count;

    return true;
}

size_t
co_array_get_count(
    const co_array_t* arr
)
{
    return arr->count;
}

void*
co_array_get_ptr(
    co_array_t* arr,
    size_t index
)
{
    co_assert(index < arr->count);

    return &arr->buffer[index * arr->element_size];
}

void
co_array_set(
    co_array_t* arr,
    size_t index,
    const void* data,
    size_t count
)
{
    if (arr->count < (index + count))
    {
        co_array_set_count(arr, (index + count));
    }

    memcpy(&arr->buffer[index * arr->element_size],
        data, count * arr->element_size);
}

size_t
co_array_get(
    const co_array_t* arr,
    size_t index,
    void* buffer,
    size_t count
)
{
    count = co_min(count, (arr->count - index));

    memcpy(buffer,
        &arr->buffer[index * arr->element_size],
        count * arr->element_size);

    return count;
}

void
co_array_add(
    co_array_t* arr,
    const void* data,
    size_t count
)
{
    co_array_set_count(arr, (arr->count + count));

    memcpy(&arr->buffer[(arr->count - count) * arr->element_size],
        data, count * arr->element_size);
}

void
co_array_zero_clear(
    co_array_t* arr
)
{
    memset(arr->buffer, 0x00,
        arr->element_size * arr->count);
}
