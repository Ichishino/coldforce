#include <coldforce/core/co_std.h>
#include <coldforce/core/co_queue.h>

//---------------------------------------------------------------------------//
// queue
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_queue_t*
co_queue_create(
    size_t element_size,
    const co_queue_ctx_t* ctx
)
{
    co_queue_t* queue =
        (co_queue_t*)co_mem_alloc(sizeof(co_queue_t));

    if (queue == NULL)
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
        capacity = CO_QUEUE_DEFAULT_CAPACITY;
    }

    queue->buffer =
        (uint8_t*)co_mem_alloc(element_size * capacity);

    if (queue->buffer == NULL)
    {
        co_mem_free(queue);

        return NULL;
    }

    queue->element_size = element_size;
    queue->capacity = capacity;

    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;

    return queue;
}

void
co_queue_destroy(
    co_queue_t* queue
)
{
    if (queue != NULL)
    {
        co_mem_free(queue->buffer);
        co_mem_free(queue);
    }
}

void
co_queue_clear(
    co_queue_t* queue
)
{
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
}

size_t
co_queue_get_count(
    const co_queue_t* queue
)
{
    return queue->count;
}

bool co_queue_set_capacity(
    co_queue_t* queue,
    size_t new_capacity
)
{
    if (queue->capacity >= new_capacity)
    {
        return true;
    }

    uint8_t* new_buffer =
        (uint8_t*)co_mem_alloc(
            queue->element_size * new_capacity);

    if (new_buffer == NULL)
    {
        return false;
    }

    size_t count1 = queue->capacity - queue->head;

    memcpy(&new_buffer[0],
        &queue->buffer[queue->head * queue->element_size],
        count1 * queue->element_size);

    size_t count2 = queue->tail;

    if (count2 > 0)
    {
        memcpy(&new_buffer[count1 * queue->element_size],
            &queue->buffer[0],
            count2 * queue->element_size);
    }

    co_mem_free(queue->buffer);

    queue->capacity = new_capacity;
    queue->buffer = new_buffer;
    queue->head = 0;
    queue->tail = queue->count;

    return true;
}

bool
co_queue_push(
    co_queue_t* queue,
    const void* value_address
)
{
    if (queue->capacity == queue->count)
    {
        size_t new_capacity = queue->capacity * 2;

        if (!co_queue_set_capacity(queue, new_capacity))
        {
            return false;
        }
    }

    memcpy(&queue->buffer[queue->tail * queue->element_size],
        value_address,
        queue->element_size);

    ++queue->tail;

    if (queue->capacity == queue->tail)
    {
        queue->tail = 0;
    }

    ++queue->count;

    return true;
}

bool
co_queue_pop(
    co_queue_t* queue,
    void* value_address
)
{
    if (queue->count == 0)
    {
        return false;
    }

    memcpy(value_address,
        &queue->buffer[queue->head * queue->element_size],
        queue->element_size);

    ++queue->head;

    if (queue->capacity == queue->head)
    {
        queue->head = 0;
    }

    --queue->count;

    return true;
}

bool
co_queue_push_array(
    co_queue_t* queue,
    const void* value_address,
    size_t count
)
{
    if (queue->capacity < (queue->count + count))
    {
        size_t new_capacity = queue->capacity * 2;

        while (new_capacity < (queue->count + count))
        {
            new_capacity *= 2;
        }

        if (!co_queue_set_capacity(queue, new_capacity))
        {
            return false;
        }
    }

    if (queue->capacity >= (queue->tail + count))
    {
        memcpy(&queue->buffer[queue->tail * queue->element_size],
            value_address,
            count * queue->element_size);
    }
    else
    {
        size_t rest_count = queue->capacity - queue->tail;

        memcpy(&queue->buffer[queue->tail * queue->element_size],
            value_address,
            rest_count * queue->element_size);

        memcpy(&queue->buffer[0],
            &((uint8_t*)value_address)[rest_count * queue->element_size],
            (count - rest_count) * queue->element_size);
    }

    queue->tail += count;

    if (queue->capacity <= queue->tail)
    {
        queue->tail -= queue->capacity;
    }

    queue->count += count;

    return true;
}

size_t
co_queue_pop_array(
    co_queue_t* queue,
    void* value_address,
    size_t count
)
{
    count = co_queue_peek_array(
        queue, value_address, count);

    co_queue_remove(queue, count);

    return count;
}

size_t
co_queue_peek_array(
    co_queue_t* queue,
    void* value_address,
    size_t count
)
{
    if (queue->count == 0)
    {
        return 0;
    }

    count = co_min(count, queue->count);

    if (queue->capacity >= (queue->head + count))
    {
        memcpy(value_address,
            &queue->buffer[queue->head * queue->element_size],
            count * queue->element_size);
    }
    else
    {
        size_t rest_count = queue->capacity - queue->head;

        memcpy(value_address,
            &queue->buffer[queue->head * queue->element_size],
            rest_count * queue->element_size);

        memcpy(&((uint8_t*)value_address)[rest_count * queue->element_size],
            &queue->buffer[0],
            (count - rest_count) * queue->element_size);
    }

    return count;
}

void
co_queue_remove(
    co_queue_t* queue,
    size_t count
)
{
    if (queue->count == 0)
    {
        return;
    }

    count = co_min(count, queue->count);

    queue->head += count;

    if (queue->capacity <= queue->head)
    {
        queue->head -= queue->capacity;
    }

    queue->count -= count;
}

void*
co_queue_peek_head(
    co_queue_t* queue
)
{
    if (queue->count == 0)
    {
        return NULL;
    }

    return &queue->buffer[queue->head * queue->element_size];
}

void*
co_queue_find(
    co_queue_t* queue,
    const void* value_address,
    co_compare_fn compare
)
{
    if (queue->count == 0)
    {
        return NULL;
    }

    size_t start = queue->head;
    size_t end = 0;

    if (queue->head >= queue->tail)
    {
        end = queue->capacity;
    }
    else
    {
        end = queue->tail;
    }

    for (size_t index = start; index < end; ++index)
    {
        void* buffer_address =
            &queue->buffer[index * queue->element_size];

        if (compare(
            (uintptr_t)value_address, (uintptr_t)buffer_address) == 0)
        {
            return buffer_address;
        }
    }

    if (queue->head >= queue->tail)
    {
        end = queue->tail;

        for (size_t index = 0; index < end; ++index)
        {
            void* buffer_address =
                &queue->buffer[index * queue->element_size];

            if (compare(
                (uintptr_t)value_address, (uintptr_t)buffer_address) == 0)
            {
                return buffer_address;
            }
        }
    }

    return NULL;
}
