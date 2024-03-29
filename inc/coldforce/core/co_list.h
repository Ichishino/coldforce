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
    co_item_destroy_fn destroy_value;
    co_item_duplicate_fn duplicate_value;
    co_item_compare_fn compare_values;

} co_list_ctx_st;

typedef struct
{
    void* value;

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

    co_item_destroy_fn destroy_value;
    co_item_compare_fn compare_values;
    co_item_duplicate_fn duplicate_value;

} co_list_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
co_list_t*
co_list_create(
    const co_list_ctx_st* ctx
);

CO_CORE_API
void
co_list_destroy(
    co_list_t* list
);

CO_CORE_API
void
co_list_clear(
    co_list_t* list
);

CO_CORE_API
size_t
co_list_get_count(
    const co_list_t* list
);

CO_CORE_API
bool
co_list_contains(
    const co_list_t* list,
    const void* value
);

CO_CORE_API
bool
co_list_add_head(
    co_list_t* list,
    void* value
);

CO_CORE_API
bool co_list_add_tail(
    co_list_t* list,
    void* value
);

CO_CORE_API
co_list_data_st*
co_list_get_head(
    co_list_t* list
);

CO_CORE_API
const co_list_data_st*
co_list_get_const_head(
    const co_list_t* list
);

CO_CORE_API
co_list_data_st*
co_list_get_tail(
    co_list_t* list
);

CO_CORE_API
const co_list_data_st*
co_list_get_const_tail(
    const co_list_t* list
);

CO_CORE_API
void
co_list_remove_head(
    co_list_t* list
);

CO_CORE_API
void
co_list_remove_tail(
    co_list_t* list
);

CO_CORE_API
void
co_list_remove(
    co_list_t* list,
    const void* value
);

CO_CORE_API
co_list_iterator_t*
co_list_get_head_iterator(
    co_list_t* list
);

CO_CORE_API
const co_list_iterator_t*
co_list_get_const_head_iterator(
    const co_list_t* list
);

CO_CORE_API
co_list_iterator_t*
co_list_get_tail_iterator(
    co_list_t* list
);

CO_CORE_API
const co_list_iterator_t*
co_list_get_const_tail_iterator(
    const co_list_t* list
);

CO_CORE_API
co_list_iterator_t*
co_list_get_prev_iterator(
    co_list_t* list,
    co_list_iterator_t* iterator
);

CO_CORE_API
const co_list_iterator_t*
co_list_get_const_prev_iterator(
    const co_list_t* list,
    const co_list_iterator_t* iterator
);

CO_CORE_API
co_list_iterator_t*
co_list_get_next_iterator(
    co_list_t* list,
    co_list_iterator_t* iterator
);

CO_CORE_API
const co_list_iterator_t*
co_list_get_const_next_iterator(
    const co_list_t* list,
    const co_list_iterator_t* iterator
);

CO_CORE_API
bool
co_list_insert(
    co_list_t* list,
    co_list_iterator_t* iterator,
    void* value
);

CO_CORE_API
bool
co_list_insert_after(
    co_list_t* list,
    co_list_iterator_t* iterator,
    void* value
);

CO_CORE_API
void
co_list_remove_at(
    co_list_t* list,
    co_list_iterator_t* iterator
);

CO_CORE_API
co_list_data_st*
co_list_get(
    co_list_t* list,
    co_list_iterator_t* iterator
);

CO_CORE_API
const co_list_data_st*
co_list_get_const(
    const co_list_t* list,
    const co_list_iterator_t* iterator
);

CO_CORE_API
co_list_data_st*
co_list_get_prev(
    co_list_t* list,
    co_list_iterator_t** iterator
);

CO_CORE_API
const co_list_data_st*
co_list_get_const_prev(
    const co_list_t* list,
    const co_list_iterator_t** iterator
);

CO_CORE_API
co_list_data_st*
co_list_get_next(
    co_list_t* list,
    co_list_iterator_t** iterator
);

CO_CORE_API
const co_list_data_st*
co_list_get_const_next(
    const co_list_t* list,
    const co_list_iterator_t** iterator
);

CO_CORE_API
co_list_iterator_t*
co_list_find(
    co_list_t* list,
    const void* value
);

CO_CORE_API
const co_list_iterator_t*
co_list_find_const(
    const co_list_t* list,
    const void* value
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_LIST_H_INCLUDED
