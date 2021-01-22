#ifndef CO_MAP_H_INCLUDED
#define CO_MAP_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_MAP_DEFAULT_HASH_SIZE    256

typedef struct
{
    size_t hash_size;

    co_item_hash_fn hash_key;
    co_item_free_fn free_key;
    co_item_free_fn free_value;
    co_item_duplicate_fn duplicate_key;
    co_item_duplicate_fn duplicate_value;
    co_item_compare_fn compare_keys;

} co_map_ctx_st;

typedef struct
{
    uintptr_t key;
    uintptr_t value;

} co_map_data_st;

typedef struct co_map_item_t
{
    co_map_data_st data;

    struct co_map_item_t* next;

} co_map_item_t;

typedef struct
{
    size_t count;

    size_t hash_size;
    co_map_item_t** items;

    co_item_hash_fn hash_key;
    co_item_free_fn free_key;
    co_item_free_fn free_value;
    co_item_duplicate_fn duplicate_key;
    co_item_duplicate_fn duplicate_value;
    co_item_compare_fn compare_keys;

} co_map_t;

typedef struct
{
    co_map_t* map;

    size_t index;
    co_map_item_t* item;

} co_map_iterator_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_map_t* co_map_create(const co_map_ctx_st* ctx);
CO_API void co_map_destroy(co_map_t* map);

CO_API void co_map_clear(co_map_t* map);
CO_API size_t co_map_get_count(const co_map_t* map);
CO_API bool co_map_contains(const co_map_t* map, uintptr_t key);

CO_API bool co_map_set(co_map_t* map, uintptr_t key, uintptr_t value);
CO_API co_map_data_st* co_map_get(co_map_t* map, uintptr_t key);
CO_API void co_map_remove(co_map_t* map, uintptr_t key);

CO_API void co_map_iterator_init(co_map_t* map, co_map_iterator_t* iterator);
CO_API co_map_data_st* co_map_iterator_get_next(co_map_iterator_t* iterator);
CO_API bool co_map_iterator_has_next(const co_map_iterator_t* iterator);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_MAP_H_INCLUDED
