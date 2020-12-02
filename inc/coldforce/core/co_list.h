#ifndef CO_LIST_H_INCLUDED
#define CO_LIST_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_free_fn free_value;
    co_duplicate_fn duplicate_value;
    co_compare_fn compare_values;

} co_list_ctx_st;

typedef struct
{
    uintptr_t value;

} co_list_data_st;

typedef struct co_list_item_t
{
    co_list_data_st data;

    struct co_list_item_t* prev;
    struct co_list_item_t* next;

} co_list_item_t, co_list_iterator_t;

typedef struct
{
    size_t count;

    co_list_item_t* head;
    co_list_item_t* tail;

    co_free_fn free_value;
    co_compare_fn compare_values;
    co_duplicate_fn duplicate_value;

} co_list_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_API co_list_t* co_list_create(const co_list_ctx_st* ctx);
CO_API void co_list_destroy(co_list_t* list);

CO_API void co_list_clear(co_list_t* list);
CO_API size_t co_list_get_count(const co_list_t* list);
CO_API bool co_list_contains(const co_list_t* list, uintptr_t value);

CO_API bool co_list_add_head(co_list_t* list, uintptr_t value);
CO_API bool co_list_add_tail(co_list_t* list, uintptr_t value);

CO_API co_list_data_st* co_list_get_head(co_list_t* list);
CO_API co_list_data_st* co_list_get_tail(co_list_t* list);

CO_API void co_list_remove_head(co_list_t* list);
CO_API void co_list_remove_tail(co_list_t* list);
CO_API void co_list_remove(co_list_t* list, uintptr_t value);

CO_API co_list_iterator_t*
    co_list_get_head_iterator(co_list_t* list);

CO_API co_list_iterator_t*
    co_list_get_tail_iterator(co_list_t* list);

CO_API co_list_iterator_t*
    co_list_get_prev_iterator(co_list_t* list, co_list_iterator_t* iterator);

CO_API co_list_iterator_t*
    co_list_get_next_iterator(co_list_t* list, co_list_iterator_t* iterator);

CO_API bool co_list_insert(
    co_list_t* list, co_list_iterator_t* iterator, uintptr_t value);

CO_API bool co_list_insert_after(
    co_list_t* list, co_list_iterator_t* iterator, uintptr_t value);

CO_API void co_list_remove_at(co_list_t* list, co_list_iterator_t* iterator);

CO_API co_list_data_st* co_list_get(
    co_list_t* list, co_list_iterator_t* iterator);

CO_API co_list_data_st* co_list_get_prev(
    co_list_t* list, co_list_iterator_t** iterator);

CO_API co_list_data_st* co_list_get_next(
    co_list_t* list, co_list_iterator_t** iterator);

CO_API co_list_iterator_t* co_list_find(
    co_list_t* list, uintptr_t value);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_LIST_H_INCLUDED
