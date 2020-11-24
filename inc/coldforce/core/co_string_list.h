#ifndef CO_STRING_LIST_H_INCLUDED
#define CO_STRING_LIST_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_list.h>
#include <coldforce/core/co_string.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// string list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    char* value;

} co_str_list_data_st;

typedef co_list_t            co_str_list_t;
typedef co_list_iterator_t   co_str_list_iterator_t;

#define CO_STR_LIST_CTX \
	{ \
		.free_value = (co_free_fn)co_mem_free, \
		.duplicate_value = (co_duplicate_fn)co_string_duplicate, \
		.compare_values = (co_compare_fn)strcmp \
	}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_str_list_destroy(list) \
    co_list_destroy(list)

#define co_str_list_clear(list) \
    co_list_clear(list)

#define co_str_list_get_size(list) \
    co_list_get_size(list)

#define co_str_list_contains(list, str) \
    co_list_contains(list, (uintptr_t)str)

#define co_str_list_add_head(list, str) \
    co_list_add_head(list, (uintptr_t)str)

#define co_str_list_add_tail(list, str) \
    co_list_add_tail(list, (uintptr_t)str)

#define co_str_list_get_head(list) \
    ((co_str_list_data_st*)co_list_get_head(list))

#define co_str_list_get_tail(list) \
    ((co_str_list_data_st*)co_list_get_tail(list))

#define co_str_list_remove_head(list) \
    co_list_remove_head(list)

#define co_str_list_remove_tail(list) \
    co_list_remove_tail(list)

#define co_str_list_get_head_iterator(list) \
    co_list_get_head_iterator(list)

#define co_str_list_get_tail_iterator(list) \
    co_list_get_tail_iterator(list)

#define co_str_list_get_prev_iterator(list, it) \
    co_list_get_prev_iterator(list, it)

#define co_str_list_get_next_iterator(list, it) \
    co_list_get_next_iterator(list, it)

#define co_str_list_insert(list, it, str) \
    co_list_insert(list, it, (uintptr_t)str)

#define co_str_list_insert_after(list, it, str) \
    co_list_insert_after(list, it, (uintptr_t)str)

#define co_str_list_remove(list, it) \
    co_list_remove(list, it)

#define co_str_list_get(list, it) \
    ((co_str_list_data_st*)co_list_get(list, it))

#define co_str_list_get_prev(list, it) \
    ((co_str_list_data_st*)co_list_get_prev(list, it))

#define co_str_list_get_next(list, it) \
    ((co_str_list_data_st*)co_list_get_next(list, it))

#define co_str_list_find(list, str) \
    co_list_find(list, (uintptr_t)str)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_LIST_H_INCLUDED
