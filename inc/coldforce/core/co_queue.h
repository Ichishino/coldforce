#ifndef CO_QUEUE_H_INCLUDED
#define CO_QUEUE_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// queue
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_QUEUE_DEFAULT_CAPACITY   8

typedef struct
{
    size_t initial_capacity;

} co_queue_ctx_t;

typedef struct
{
    size_t size;
    size_t element_length;

    size_t head;
    size_t tail;

    size_t capacity;
    uint8_t* buffer;

} co_queue_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_queue_t* co_queue_create(
    size_t element_length, const co_queue_ctx_t* ctx);

CO_API void co_queue_destroy(co_queue_t* queue);

CO_API void co_queue_clear(co_queue_t* queue);
CO_API size_t co_queue_get_size(const co_queue_t* queue);

CO_API bool co_queue_push(co_queue_t* queue, const void* value_address);
CO_API bool co_queue_pop(co_queue_t* queue, void* value_address);

CO_API void* co_queue_find(
    co_queue_t* queue, const void* value_address, co_compare_fn compare);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_QUEUE_H_INCLUDED