#ifndef CO_STRING_MAP_H_INCLUDED
#define CO_STRING_MAP_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_map.h>
#include <coldforce/core/co_string.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string-key string-value map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    char* key;
    char* value;

} co_string_map_data_st;

typedef co_map_t            co_string_map_t;
typedef co_map_iterator_t   co_string_map_iterator_t;
typedef co_map_const_iterator_t   co_string_map_const_iterator_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
co_string_map_t*
co_string_map_create(
    void
);

#define co_string_map_destroy(map)       co_map_destroy(map)
#define co_string_map_clear(map)         co_map_clear(map)
#define co_string_map_get_count(map)     co_map_get_count(map)

#define co_string_map_contains(map, key) \
    co_map_contains(map, (const void*)key)

#define co_string_map_set(map, key, value) \
    co_map_set(map, (void*)key, (void*)value)

#define co_string_map_get(map, key) \
    ((co_string_map_data_st*)co_map_get(map, (const void*)key))

#define co_string_map_remove(map, key) \
    co_map_remove(map, (const void*)key)

#define co_string_map_iterator_init \
    co_map_iterator_init

#define co_string_map_const_iterator_init \
    co_map_const_iterator_init

#define co_string_map_iterator_get_next(iterator) \
    ((co_string_map_data_st*)co_map_iterator_get_next(iterator))

#define co_string_map_const_iterator_get_next(iterator) \
    ((const co_string_map_data_st*)co_map_const_iterator_get_next(iterator))

#define co_string_map_iterator_has_next \
    co_map_iterator_has_next

#define co_string_map_const_iterator_has_next \
    co_map_const_iterator_has_next

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_MAP_H_INCLUDED
