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
    co_array_create(sizeof(uint8_t))

// void co_byte_array_destroy(co_byte_array_t* arr)
#define co_byte_array_destroy(arr) \
    co_array_destroy(arr)

// uint8_t* co_byte_array_detach(co_byte_array_t* arr)
#define co_byte_array_detach(arr) \
    ((uint8_t*)co_array_detach(arr))

// void co_byte_array_clear(co_byte_array_t* arr)
#define co_byte_array_clear(arr) \
    co_array_clear(arr)

// bool co_byte_array_set_count(co_byte_array_t* arr, size_t size)
#define co_byte_array_set_count(arr, size) \
    co_array_set_count(arr, size)

// size_t co_byte_array_get_count(const co_byte_array_t* arr)
#define co_byte_array_get_count(arr) \
    co_array_get_count(arr)

// uint8_t* co_byte_array_get_ptr(co_byte_array_t* arr, size_t index)
#define co_byte_array_get_ptr(arr, index) \
    ((uint8_t*)co_array_get_ptr(arr, index))

// const uint8_t* co_byte_array_get_const_ptr(const co_byte_array_t* arr, size_t index)
#define co_byte_array_get_const_ptr(arr, index) \
    ((const uint8_t*)co_array_get_const_ptr(arr, index))

// void co_byte_array_set(
//     co_byte_array_t* arr, size_t index, const void* data, size_t count)
#define co_byte_array_set(arr, index, data, count) \
    co_array_set(arr, index, data, count)

// size_t co_byte_array_get(
//     const co_byte_array_t* arr, size_t index, void* buffer, size_t count)
#define co_byte_array_get(arr, index, buffer, count) \
    co_array_get(arr, index, buffer, count)

// void co_byte_array_add(
//     co_byte_array_t* arr, const void* data, size_t count)
#define co_byte_array_add(arr, data, count) \
    co_array_add(arr, data, count)

// void co_byte_array_add_string(
//     co_byte_array_t* arr, const char* str)
#define co_byte_array_add_string(arr, str) \
    co_array_add(arr, str, strlen(str))

// void co_byte_array_zero_clear(co_byte_array_t* arr)
#define co_byte_array_zero_clear(arr) \
    co_array_zero_clear(arr)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_BYTE_ARRAY_H_INCLUDED
