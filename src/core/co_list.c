#include <coldforce/core/co_std.h>
#include <coldforce/core/co_list.h>

//---------------------------------------------------------------------------//
// list
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_list_default_free_value(
    uintptr_t value
)
{
    (void)value;
}

uintptr_t
co_list_default_duplicate_value(
    uintptr_t value
)
{
    return value;
}

intptr_t
co_list_default_compare_values(
    uintptr_t value1,
    uintptr_t value2
)
{
    return (((intptr_t)value1) - ((intptr_t)value2));
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_list_t*
co_list_create(
    const co_list_ctx_st* ctx
)
{
    co_list_t* list =
        (co_list_t*)co_mem_alloc(sizeof(co_list_t));

    if (list == NULL)
    {
        return NULL;
    }

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;

    if (ctx != NULL)
    {
        list->free_value = ctx->free_value;
        list->duplicate_value = ctx->duplicate_value;
        list->compare_values = ctx->compare_values;
    }
    else
    {
        list->free_value = NULL;
        list->duplicate_value = NULL;
        list->compare_values = NULL;
    }

    if (list->free_value == NULL)
    {
        list->free_value = co_list_default_free_value;
    }

    if (list->duplicate_value == NULL)
    {
        list->duplicate_value = co_list_default_duplicate_value;
    }

    if (list->compare_values == NULL)
    {
        list->compare_values = co_list_default_compare_values;
    }

    return list;
}

void
co_list_destroy(
    co_list_t* list
)
{
    if (list != NULL)
    {
        co_list_clear(list);
        co_mem_free(list);
    }
}

void
co_list_clear(
    co_list_t* list
)
{
    co_list_item_t* item = list->head;

    while (item != NULL)
    {
        co_list_item_t* temp = item;
        item = item->next;

        list->free_value(temp->data.value);
        
        co_mem_free(temp);
    }

    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
}

size_t
co_list_get_size(
    const co_list_t* list
)
{
    return list->size;
}

bool
co_list_contains(
    const co_list_t* list,
    uintptr_t value
)
{
    const co_list_item_t* item = list->head;

    while (item != NULL)
    {
        if (list->compare_values(item->data.value, value) == 0)
        {
            return true;
        }

        item = item->next;
    }

    return false;
}

bool
co_list_add_head(
    co_list_t* list,
    uintptr_t value
)
{
    co_list_item_t* new_item =
        (co_list_item_t*)co_mem_alloc(sizeof(co_list_item_t));

    if (new_item == NULL)
    {
        return false;
    }

    new_item->data.value = list->duplicate_value(value);
    new_item->prev = NULL;

    if (list->head != NULL)
    {
        new_item->next = list->head;
        list->head->prev = new_item;
    }
    else
    {
        new_item->next = NULL;
        list->tail = new_item;
    }

    list->head = new_item;

    ++list->size;

    return false;
}

bool
co_list_add_tail(
    co_list_t* list,
    uintptr_t value
)
{
    co_list_item_t* new_item =
        (co_list_item_t*)co_mem_alloc(sizeof(co_list_item_t));

    if (new_item == NULL)
    {
        return false;
    }

    new_item->data.value = list->duplicate_value(value);
    new_item->next = NULL;

    if (list->tail != NULL)
    {
        list->tail->next = new_item;
        new_item->prev = list->tail;
    }
    else
    {
        list->head = new_item;
        new_item->prev = NULL;
    }

    list->tail = new_item;

    ++list->size;

    return true;
}

co_list_data_st*
co_list_get_head(
    co_list_t* list
)
{
    return ((list->head != NULL) ? &list->head->data : NULL);
}

co_list_data_st*
co_list_get_tail(
    co_list_t* list
)
{
    return ((list->tail != NULL) ? &list->tail->data : NULL);
}

void
co_list_remove_head(
    co_list_t* list
)
{
    if (list->head != NULL)
    {
        co_list_item_t* item = list->head;

        list->head = item->next;
        list->free_value(item->data.value);

        if (list->head != NULL)
        {
            list->head->prev = NULL;
        }

        co_mem_free(item);

        --list->size;

        if (list->size == 0)
        {
            list->head = NULL;
            list->tail = NULL;
        }
    }
}

void
co_list_remove_tail(
    co_list_t* list
)
{
    if (list->tail != NULL)
    {
        co_list_item_t* item = list->tail;

        list->tail = item->prev;
        list->free_value(item->data.value);

        if (list->tail != NULL)
        {
            list->tail->next = NULL;
        }

        co_mem_free(item);

        --list->size;

        if (list->size == 0)
        {
            list->head = NULL;
            list->tail = NULL;
        }
    }
}

void
co_list_remove(
    co_list_t* list,
    co_list_iterator_t* iterator
)
{
    if (list->head == iterator)
    {
        co_list_remove_head(list);
    }
    else if (list->tail == iterator)
    {
        co_list_remove_tail(list);
    }
    else
    {
        iterator->prev->next = iterator->next;
        iterator->next->prev = iterator->prev;

        list->free_value(iterator->data.value);

        co_mem_free(iterator);

        --list->size;
    }
}

co_list_iterator_t*
co_list_get_head_iterator(
    co_list_t* list
)
{
    return list->head;
}

co_list_iterator_t*
co_list_get_tail_iterator(
    co_list_t* list
)
{
    return list->tail;
}

co_list_iterator_t*
co_list_get_prev_iterator(
    co_list_t* list,
    co_list_iterator_t* iterator
)
{
    (void)list;

    return iterator->prev;
}

co_list_iterator_t*
co_list_get_next_iterator(
    co_list_t* list,
    co_list_iterator_t* iterator
)
{
    (void)list;

    return iterator->next;
}

bool
co_list_insert(
    co_list_t* list,
    co_list_iterator_t* iterator,
    uintptr_t value
)
{
    co_list_item_t* new_item =
        (co_list_item_t*)co_mem_alloc(sizeof(co_list_item_t));

    if (new_item == NULL)
    {
        return false;
    }

    new_item->data.value = list->duplicate_value(value);
    new_item->prev = iterator->prev;
    new_item->next = iterator;
    iterator->prev = new_item;

    if (new_item->prev != NULL)
    {
        new_item->prev->next = new_item;
    }

    if (list->head == iterator)
    {
        list->head = new_item;
    }

    ++list->size;

    return true;
}

bool
co_list_insert_after(
    co_list_t* list,
    co_list_iterator_t* iterator,
    uintptr_t value
)
{
    co_list_item_t* new_item =
        (co_list_item_t*)co_mem_alloc(sizeof(co_list_item_t));

    if (new_item == NULL)
    {
        return false;
    }

    new_item->data.value = list->duplicate_value(value);
    new_item->prev = iterator;
    new_item->next = iterator->next;
    iterator->next = new_item;

    if (new_item->next != NULL)
    {
        new_item->next->prev = new_item;
    }

    if (list->tail == iterator)
    {
        list->tail = new_item;
    }

    ++list->size;

    return true;
}

co_list_data_st*
co_list_get(
    co_list_t* list,
    co_list_iterator_t* iterator
)
{
    (void)list;

    return &iterator->data;
}

co_list_data_st*
co_list_get_prev(
    co_list_t* list,
    co_list_iterator_t** iterator
)
{
    (void)list;

    co_list_data_st* data = &(*iterator)->data;

    (*iterator) = (*iterator)->prev;

    return data;
}

co_list_data_st*
co_list_get_next(
    co_list_t* list,
    co_list_iterator_t** iterator
)
{
    (void)list;

    co_list_data_st* data = &(*iterator)->data;

    (*iterator) = (*iterator)->next;

    return data;
}

co_list_iterator_t*
co_list_find(
    co_list_t* list,
    uintptr_t value
)
{
    co_list_item_t* item = list->head;

    while (item != NULL)
    {
        if (list->compare_values(item->data.value, value) == 0)
        {
            return item;
        }

        item = item->next;
    }

    return NULL;
}
