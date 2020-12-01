#ifndef CO_BYTE_ARRAY_H_INCLUDED
#define CO_BYTE_ARRAY_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_array.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// byte array
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef co_array_t co_byte_array_t;

// co_byte_array_t* co_byte_array_create(void)
#define co_byte_array_create() \
    co_array_create(sizeof(uint8_t), NULL)

// void co_byte_array_destroy(co_byte_array_t* arr)
#define co_byte_array_destroy(arr) \
    co_array_destroy(arr)

// bool co_byte_array_set_size(co_byte_array_t* arr, size_t size)
#define co_byte_array_set_size(arr, size) \
    co_array_set_count(arr, size)

// size_t co_byte_array_get_size(const co_byte_array_t* arr)
#define co_byte_array_get_size(arr) \
    co_array_get_count(arr)

// uint8_t* co_byte_array_get_ptr(co_byte_array_t* arr, size_t index)
#define co_byte_array_get_ptr(arr, index) \
    ((uint8_t*)co_array_get_ptr(arr, index))

// void co_byte_array_set(
//     co_byte_array_t* arr, size_t index, void* data, size_t count)
#define co_byte_array_set(arr, index, data, count) \
    co_array_set(arr, index, data, count)

// size_t co_byte_array_get(
//     const co_byte_array_t* arr, size_t index, void* buffer, size_t count)
#define co_byte_array_get(arr, index, buffer, count) \
    co_array_get(arr, index, buffer, count)

// void co_byte_array_add(
//     co_byte_array_t* arr, void* data, size_t count)
#define co_byte_array_add(arr, data, count) \
    co_array_add(arr, data, count)

// void co_byte_array_zero_clear(co_byte_array_t* arr)
#define co_byte_array_zero_clear(arr) \
    co_array_zero_clear(arr)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_BYTE_ARRAY_H_INCLUDED
