#ifndef CO_ARRAY_H_INCLUDED
#define CO_ARRAY_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// array
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_ARRAY_DEFAULT_CAPACITY   8

typedef struct
{
    size_t initial_capacity;

} co_array_ctx_t;

typedef struct {

    size_t capacity;
    size_t size;
    size_t element_length;

    uint8_t* buffer;

} co_array_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_array_t* co_array_create(
    size_t element_length, co_array_ctx_t* ctx);

CO_API void co_array_destroy(co_array_t* arr);

CO_API bool co_array_set_size(co_array_t* arr, size_t size);
CO_API size_t co_array_get_size(const co_array_t* arr);

CO_API void* co_array_get(co_array_t* arr, size_t index);

CO_API void co_array_zero_clear(co_array_t* arr);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_ARRAY_H_INCLUDED
