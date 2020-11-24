#include <coldforce/core/co_std.h>
#include <coldforce/core/co_queue.h>

//---------------------------------------------------------------------------//
// queue
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_queue_t*
co_queue_create(
    size_t element_length,
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
        (uint8_t*)co_mem_alloc(element_length * capacity);

    if (queue->buffer == NULL)
    {
        co_mem_free(queue);

        return NULL;
    }

    queue->element_length = element_length;
    queue->capacity = capacity;

    queue->size = 0;
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
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
}

size_t
co_queue_get_size(
    const co_queue_t* queue
)
{
    return queue->size;
}

bool
co_queue_push(
    co_queue_t* queue,
    const void* value_address
)
{
    if (queue->capacity == queue->size)
    {
        size_t new_capacity = queue->capacity * 2;

        uint8_t* new_buffer =
            (uint8_t*)co_mem_alloc(
                queue->element_length * new_capacity);

        if (new_buffer == NULL)
        {
            return false;
        }

        size_t length1 = queue->capacity - queue->head;
    
        memcpy(&new_buffer[0],
            &queue->buffer[queue->head * queue->element_length],
            length1 * queue->element_length);

        size_t length2 = queue->tail;

        if (length2 > 0)
        {
            memcpy(&new_buffer[length1 * queue->element_length],
                &queue->buffer[0],
                length2 * queue->element_length);
        }
        
        co_mem_free(queue->buffer);

        queue->capacity = new_capacity;
        queue->buffer = new_buffer;
        queue->head = 0;
        queue->tail = queue->size;
    }

    memcpy(&queue->buffer[queue->tail * queue->element_length],
        value_address,
        queue->element_length);

    ++queue->tail;

    if (queue->capacity == queue->tail)
    {
        queue->tail = 0;
    }

    ++queue->size;

    return true;
}

bool
co_queue_pop(
    co_queue_t* queue,
    void* value_address
)
{
    if (queue->size == 0)
    {
        return false;
    }

    memcpy(value_address,
        &queue->buffer[queue->head * queue->element_length],
        queue->element_length);

    ++queue->head;

    if (queue->capacity == queue->head)
    {
        queue->head = 0;
    }

    --queue->size;

    return true;
}

void*
co_queue_peek_head(
    co_queue_t* queue
)
{
    if (queue->size == 0)
    {
        return NULL;
    }

    return &queue->buffer[queue->head * queue->element_length];
}

void*
co_queue_find(
    co_queue_t* queue,
    const void* value_address,
    co_compare_fn compare
)
{
    if (queue->size == 0)
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
            &queue->buffer[index * queue->element_length];

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
                &queue->buffer[index * queue->element_length];

            if (compare(
                (uintptr_t)value_address, (uintptr_t)buffer_address) == 0)
            {
                return buffer_address;
            }
        }
    }

    return NULL;
}
