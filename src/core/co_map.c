#include <coldforce/core/co_std.h>
#include <coldforce/core/co_map.h>

//---------------------------------------------------------------------------//
// map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static size_t
co_map_default_hash_key(
    const void* key
)
{
    return (size_t)key;
}

static void
co_map_default_destroy(
    void* key_or_value
)
{
    (void)key_or_value;
}

static void*
co_map_default_duplicate(
    const void* key_or_value
)
{
    return (void*)key_or_value;
}

static int
co_map_default_compare_keys(
    const void* key1,
    const void* key2
)
{
    return (int)(((intptr_t)key1) - ((intptr_t)key2));
}

//---------------------------------------------------------------------------//
// public
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

    map->count = 0;

    if (ctx != NULL)
    {
        map->hash_size = ctx->hash_size;
        map->hash_key = ctx->hash_key;
        map->destroy_key = ctx->destroy_key;
        map->destroy_value = ctx->destroy_value;
        map->duplicate_key = ctx->duplicate_key;
        map->duplicate_value = ctx->duplicate_value;
        map->compare_keys = ctx->compare_keys;
    }
    else
    {
        map->hash_size = 0;
        map->hash_key = NULL;
        map->destroy_key = NULL;
        map->destroy_value = NULL;
        map->duplicate_key = NULL;
        map->duplicate_value = NULL;
        map->compare_keys = NULL;
    }

    if (map->hash_size == 0)
    {
        map->hash_size = CO_MAP_DEFAULT_HASH_SIZE;
    }

    if (map->hash_key == NULL)
    {
        map->hash_key = co_map_default_hash_key;
    }

    if (map->destroy_key == NULL)
    {
        map->destroy_key = co_map_default_destroy;
    }

    if (map->destroy_value == NULL)
    {
        map->destroy_value = co_map_default_destroy;
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
        co_mem_alloc(sizeof(co_map_item_t*) * map->hash_size);

    if (map->items == NULL)
    {
        co_mem_free(map);

        return NULL;
    }

    for (size_t index = 0; index < map->hash_size; ++index)
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
    for (size_t index = 0; index < map->hash_size; ++index)
    {
        if (map->items[index] != NULL)
        {
            co_map_item_t* item = map->items[index];
            map->items[index] = NULL;

            do
            {
                map->destroy_key(item->data.key);
                map->destroy_value(item->data.value);

                co_map_item_t* temp = item;
                item = item->next;

                co_mem_free(temp);

            } while (item != NULL);
        }
    }

    map->count = 0;
}

size_t
co_map_get_count(
    const co_map_t* map
)
{
    return map->count;
}

bool
co_map_contains(
    const co_map_t* map,
    const void* key
)
{
    size_t index = map->hash_key(key) % map->hash_size;

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
    void* key,
    void* value
)
{
    size_t index = map->hash_key(key) % map->hash_size;

    co_map_item_t** item = &map->items[index];

    while ((*item) != NULL)
    {
        if (map->compare_keys((*item)->data.key, key) == 0)
        {
            map->destroy_value((*item)->data.value);
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

    ++map->count;

    return true;
}

co_map_data_st*
co_map_get(
    co_map_t* map,
    const void* key
)
{
    size_t index = map->hash_key(key) % map->hash_size;

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
    const void* key
)
{
    size_t index = map->hash_key(key) % map->hash_size;

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

            map->destroy_key(item->data.key);
            map->destroy_value(item->data.value);

            co_mem_free(item);

            --map->count;

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

    for (size_t index = 0; index < map->hash_size; ++index)
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

void
co_map_const_iterator_init(
    const co_map_t* map,
    co_map_const_iterator_t* iterator
)
{
    iterator->map = map;

    for (size_t index = 0; index < map->hash_size; ++index)
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
            index < iterator->map->hash_size;
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

const co_map_data_st*
co_map_const_iterator_get_next(
    co_map_const_iterator_t* iterator
)
{
    const co_map_data_st* data = &iterator->item->data;

    if (iterator->item->next != NULL)
    {
        iterator->item = iterator->item->next;

        return data;
    }
    else
    {
        for (size_t index = iterator->index + 1;
            index < iterator->map->hash_size;
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
    co_map_iterator_t* iterator
)
{
    return (iterator->item != NULL) &&
        (iterator->map->hash_size > iterator->index);
}

bool
co_map_const_iterator_has_next(
    co_map_const_iterator_t* iterator
)
{
    return (iterator->item != NULL) &&
        (iterator->map->hash_size > iterator->index);
}
