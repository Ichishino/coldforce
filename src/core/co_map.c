#include <coldforce/core/co_std.h>
#include <coldforce/core/co_map.h>

//---------------------------------------------------------------------------//
// map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

size_t
co_map_default_hash_key(
    uintptr_t key
)
{
    return (size_t)key;
}

void
co_map_default_free(
    uintptr_t key_or_value
)
{
    (void)key_or_value;
}

uintptr_t
co_map_default_duplicate(
    uintptr_t key_or_value
)
{
    return key_or_value;
}

intptr_t
co_map_default_compare_keys(
    uintptr_t key1,
    uintptr_t key2
)
{
    return (((intptr_t)key1) - ((intptr_t)key2));
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_map_t*
co_map_create(
    const co_map_ctx_st* ctx
)
{
    co_map_t* map = (co_map_t*)co_mem_alloc(sizeof(co_map_t));

    if (map == NULL)
    {
        return NULL;
    }

    map->size = 0;

    if (ctx != NULL)
    {
        map->hash_length = ctx->hash_length;
        map->hash_key = ctx->hash_key;
        map->free_key = ctx->free_key;
        map->free_value = ctx->free_value;
        map->duplicate_key = ctx->duplicate_key;
        map->duplicate_value = ctx->duplicate_value;
        map->compare_keys = ctx->compare_keys;
    }
    else
    {
        map->hash_length = 0;
        map->hash_key = NULL;
        map->free_key = NULL;
        map->free_value = NULL;
        map->duplicate_key = NULL;
        map->duplicate_value = NULL;
        map->compare_keys = NULL;
    }

    if (map->hash_length == 0)
    {
        map->hash_length = CO_MAP_DEFAULT_HASH_LENGTH;
    }

    if (map->hash_key == NULL)
    {
        map->hash_key = co_map_default_hash_key;
    }

    if (map->free_key == NULL)
    {
        map->free_key = co_map_default_free;
    }

    if (map->free_value == NULL)
    {
        map->free_value = co_map_default_free;
    }

    if (map->duplicate_key == NULL)
    {
        map->duplicate_key = co_map_default_duplicate;
    }

    if (map->duplicate_value == NULL)
    {
        map->duplicate_value = co_map_default_duplicate;
    }

    if (map->compare_keys == NULL)
    {
        map->compare_keys = co_map_default_compare_keys;
    }

    map->items = (co_map_item_t**)
        co_mem_alloc(sizeof(co_map_item_t*) * map->hash_length);

    if (map->items == NULL)
    {
        co_mem_free(map);

        return NULL;
    }

    for (size_t index = 0; index < map->hash_length; ++index)
    {
        map->items[index] = NULL;
    }

    return map;
}

void
co_map_destroy(
    co_map_t* map
)
{
    if (map != NULL)
    {
        co_map_clear(map);
        co_mem_free(map->items);
        co_mem_free(map);
    }
}

void
co_map_clear(
    co_map_t* map
)
{
    for (size_t index = 0; index < map->hash_length; ++index)
    {
        if (map->items[index] != NULL)
        {
            co_map_item_t* item = map->items[index];
            map->items[index] = NULL;

            do
            {
                map->free_key(item->data.key);
                map->free_value(item->data.value);

                co_map_item_t* temp = item;
                item = item->next;

                co_mem_free(temp);

            } while (item != NULL);
        }
    }

    map->size = 0;
}

size_t
co_map_get_size(
    const co_map_t* map
)
{
    return map->size;
}

bool
co_map_contains(
    const co_map_t* map,
    uintptr_t key
)
{
    size_t index = map->hash_key(key) % map->hash_length;

    const co_map_item_t* item = map->items[index];

    while (item != NULL)
    {
        if (map->compare_keys(item->data.key, key) == 0)
        {
            return true;
        }

        item = item->next;
    }

    return false;
}

bool
co_map_set(
    co_map_t* map,
    uintptr_t key,
    uintptr_t value
)
{
    size_t index = map->hash_key(key) % map->hash_length;

    co_map_item_t** item = &map->items[index];

    while ((*item) != NULL)
    {
        if (map->compare_keys((*item)->data.key, key) == 0)
        {
            map->free_value((*item)->data.value);
            (*item)->data.value = map->duplicate_value(value);

            return true;
        }

        item = &(*item)->next;
    }

    (*item) = (co_map_item_t*)
        co_mem_alloc(sizeof(co_map_item_t));

    if ((*item) == NULL)
    {
        return false;
    }

    (*item)->data.key = map->duplicate_key(key);
    (*item)->data.value = map->duplicate_value(value);
    (*item)->next = NULL;

    ++map->size;

    return true;
}

co_map_data_st*
co_map_get(
    co_map_t* map,
    uintptr_t key
)
{
    size_t index = map->hash_key(key) % map->hash_length;

    co_map_item_t* item = map->items[index];

    while (item != NULL)
    {
        if (map->compare_keys(item->data.key, key) == 0)
        {
            return &item->data;
        }

        item = item->next;
    }

    return NULL;
}

void
co_map_remove(
    co_map_t* map,
    uintptr_t key
)
{
    size_t index = map->hash_key(key) % map->hash_length;

    co_map_item_t* item = map->items[index];
    co_map_item_t* prev = NULL;

    while (item != NULL)
    {
        if (map->compare_keys(item->data.key, key) == 0)
        {
            if (prev != NULL)
            {
                prev->next = item->next;
            }
            else
            {
                map->items[index] = NULL;
            }

            map->free_key(item->data.key);
            map->free_value(item->data.value);

            co_mem_free(item);

            --map->size;

            break;
        }

        prev = item;
        item = item->next;
    }
}

void
co_map_iterator_init(
    co_map_t* map,
    co_map_iterator_t* iterator
)
{
    iterator->map = map;

    for (size_t index = 0; index < map->hash_length; ++index)
    {
        if (map->items[index] != NULL)
        {
            iterator->index = index;
            iterator->item = map->items[index];

            return;
        }
    }

    iterator->index = SIZE_MAX;
    iterator->item = NULL;
}

co_map_data_st*
co_map_iterator_get_next(
    co_map_iterator_t* iterator
)
{
    co_map_data_st* data = &iterator->item->data;

    if (iterator->item->next != NULL)
    {
        iterator->item = iterator->item->next;

        return data;
    }
    else
    {
        for (size_t index = iterator->index + 1;
            index < iterator->map->hash_length;
            ++index)
        {
            if (iterator->map->items[index] != NULL)
            {
                iterator->index = index;
                iterator->item = iterator->map->items[index];

                return data;
            }
        }
    }

    iterator->index = SIZE_MAX;
    iterator->item = NULL;

    return data;
}

bool
co_map_iterator_has_next(
    const co_map_iterator_t* iterator
)
{
    return (iterator->item != NULL) &&
        (iterator->map->hash_length > iterator->index);
}
