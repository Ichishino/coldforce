#ifndef CO_ARRAY_H_INCLUDED
#define CO_ARRAY_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// array
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct {

    size_t capacity;
    size_t count;
    size_t element_size;

    uint8_t* buffer;

} co_array_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_API
co_array_t*
co_array_create(
    size_t element_size
);

CO_API
void
co_array_destroy(
    co_array_t* arr
);

CO_API
void*
co_array_detach(
    co_array_t* arr
);

CO_API
void
co_array_clear(
    co_array_t* arr
);

CO_API
bool
co_array_set_count(
    co_array_t* arr,
    size_t count
);

CO_API
size_t
co_array_get_count(
    const co_array_t* arr
);

CO_API
void*
co_array_get_ptr(
    co_array_t* arr,
    size_t index
);

CO_API
const void*
co_array_get_const_ptr(
    const co_array_t* arr,
    size_t index
);

CO_API
void
co_array_set(
    co_array_t* arr,
    size_t index,
    const void* data,
    size_t count
);

CO_API
size_t
co_array_get(
    const co_array_t* arr,
    size_t index,
    void* buffer,
    size_t count
);

CO_API
void
co_array_add(
    co_array_t* arr,
    const void* data,
    size_t count
);

CO_API
void
co_array_zero_clear(
    co_array_t* arr
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_ARRAY_H_INCLUDED
