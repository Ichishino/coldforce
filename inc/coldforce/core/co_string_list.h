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

} co_string_list_data_st;

typedef co_list_t            co_string_list_t;
typedef co_list_iterator_t   co_string_list_iterator_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
co_string_list_t*
co_string_list_create(
    void
);

#define co_string_list_destroy(list) \
    co_list_destroy(list)

#define co_string_list_clear(list) \
    co_list_clear(list)

#define co_string_list_get_count(list) \
    co_list_get_count(list)

#define co_string_list_contains(list, str) \
    co_list_contains(list, str)

#define co_string_list_add_head(list, str) \
    co_list_add_head(list, str)

#define co_string_list_add_tail(list, str) \
    co_list_add_tail(list, str)

#define co_string_list_get_head(list) \
    ((co_string_list_data_st*)co_list_get_head(list))

#define co_string_list_get_const_head(list) \
    ((const co_string_list_data_st*)co_list_get_const_head(list))

#define co_string_list_get_tail(list) \
    ((co_string_list_data_st*)co_list_get_tail(list))

#define co_string_list_get_const_tail(list) \
    ((const co_string_list_data_st*)co_list_get_const_tail(list))

#define co_string_list_remove_head(list) \
    co_list_remove_head(list)

#define co_string_list_remove_tail(list) \
    co_list_remove_tail(list)

#define co_string_list_get_head_iterator(list) \
    co_list_get_head_iterator(list)

#define co_string_list_get_const_head_iterator(list) \
    co_list_get_const_head_iterator(list)

#define co_string_list_get_tail_iterator(list) \
    co_list_get_tail_iterator(list)

#define co_string_list_get_const_tail_iterator(list) \
    co_list_get_const_tail_iterator(list)

#define co_string_list_get_prev_iterator(list, it) \
    co_list_get_prev_iterator(list, it)

#define co_string_list_get_const_prev_iterator(list, it) \
    co_list_get_const_prev_iterator(list, it)

#define co_string_list_get_next_iterator(list, it) \
    co_list_get_next_iterator(list, it)

#define co_string_list_get_const_next_iterator(list, it) \
    co_list_get_const_next_iterator(list, it)

#define co_string_list_insert(list, it, str) \
    co_list_insert(list, it, str)

#define co_string_list_insert_after(list, it, str) \
    co_list_insert_after(list, it, str)

#define co_string_list_remove(list, it) \
    co_list_remove(list, it)

#define co_string_list_get(list, it) \
    ((co_string_list_data_st*)co_list_get(list, it))

#define co_string_list_get_const(list, it) \
    ((const co_string_list_data_st*)co_list_get_const(list, it))

#define co_string_list_get_prev(list, it) \
    ((co_string_list_data_st*)co_list_get_prev(list, it))

#define co_string_list_get_const_prev(list, it) \
    ((const co_string_list_data_st*)co_list_get_const_prev(list, it))

#define co_string_list_get_next(list, it) \
    ((co_string_list_data_st*)co_list_get_next(list, it))

#define co_string_list_get_const_next(list, it) \
    ((const co_string_list_data_st*)co_list_get_const_next(list, it))

#define co_string_list_find(list, str) \
    co_list_find(list, str)

#define co_string_list_find_const(list, str) \
    co_list_find_const(list, str)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_STRING_LIST_H_INCLUDED
