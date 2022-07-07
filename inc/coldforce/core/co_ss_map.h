#ifndef CO_SS_MAP_H_INCLUDED
#define CO_SS_MAP_H_INCLUDED

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

} co_ss_map_data_st;

typedef co_map_t            co_ss_map_t;
typedef co_map_iterator_t   co_ss_map_iterator_t;

#define CO_SS_MAP_CTX \
    { \
        .hash_size = CO_MAP_DEFAULT_HASH_SIZE, \
        .hash_key = (co_item_hash_fn)co_string_hash, \
        .destroy_key = (co_item_destroy_fn)co_string_destroy, \
        .destroy_value = (co_item_destroy_fn)co_string_destroy, \
        .duplicate_key = (co_item_duplicate_fn)co_string_duplicate, \
        .duplicate_value = (co_item_duplicate_fn)co_string_duplicate, \
        .compare_keys = (co_item_compare_fn)strcmp \
    }

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

#define co_ss_map_destroy       co_map_destroy
#define co_ss_map_clear         co_map_clear
#define co_ss_map_get_size      co_map_get_size

#define co_ss_map_contains(map, key) \
    co_map_contains(map, (uintptr_t)key)

#define co_ss_map_set(map, key, value) \
    co_map_set(map, (uintptr_t)key, (uintptr_t)value)

#define co_ss_map_get(map, key) \
    ((co_ss_map_data_st*)co_map_get(map, (uintptr_t)key))

#define co_ss_map_remove(map, key) \
    co_map_remove(map, (uintptr_t)key)

#define co_ss_map_iterator_init co_map_iterator_init

#define co_ss_map_iterator_get_next(iterator) \
    ((co_ss_map_data_st*)co_map_iterator_get_next(iterator))

#define co_ss_map_iterator_has_next co_map_iterator_has_next

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SS_MAP_H_INCLUDED
