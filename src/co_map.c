#include <coldforce/co_map.h>

//---------------------------------------------------------------------------//
// Map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

size_t CO_MapHash(const uintptr_t key)
{
    return (size_t)key;
}

bool CO_MapCompareKeys(const uintptr_t key1, const uintptr_t key2)
{
    return (key1 == key2);
}

void CO_MapDeleteKey(uintptr_t key)
{
    (void)key;
}

void CO_MapDeleteValue(uintptr_t value)
{
    (void)value;
}

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_MAP_T* CO_MapCreate(const CO_MAP_PARAM_ST* param)
{
    CO_MAP_T* map = (CO_MAP_T*)CO_MemAlloc(sizeof(CO_MAP_T));

    if (param != NULL)
    {
        map->maxHash = param->maxHash;
        map->hash = param->hash;
        map->compareKeys = param->compareKeys;
        map->deleteKey = param->deleteKey;
        map->deleteValue = param->deleteValue;
    }
    else
    {
        map->maxHash = 0;
        map->hash = NULL;
        map->compareKeys = NULL;
        map->deleteKey = NULL;
        map->deleteValue = NULL;
    }

    if (map->maxHash == 0)
    {
        map->maxHash = 64;
    }

    if (map->hash == NULL)
    {
        map->hash = CO_MapHash;
    }

    if (map->compareKeys == NULL)
    {
        map->compareKeys = CO_MapCompareKeys;
    }

    if (map->deleteKey == NULL)
    {
        map->deleteKey = CO_MapDeleteKey;
    }

    if (map->deleteValue == NULL)
    {
        map->deleteValue = CO_MapDeleteValue;
    }

    map->items =
        (CO_MAP_ITEM_T**)CO_MemAlloc(sizeof(CO_MAP_ITEM_T*) * map->maxHash);

    for (size_t index = 0; index < map->maxHash; ++index)
    {
        map->items[index] = NULL;
    }

    return map;
}

void CO_MapClear(CO_MAP_T* map)
{
    CO_Assert(map != NULL);

    for (size_t index = 0; index < map->maxHash; ++index)
    {
        if (map->items[index] != NULL)
        {
            CO_MAP_ITEM_T* head = map->items[index];
            map->items[index] = NULL;

            do
            {
                CO_MAP_ITEM_T* item = head;

                map->deleteKey(item->key);
                map->deleteValue(item->value);

                head = head->next;

                CO_MemFree(item);

            } while (head != NULL);
        }
    }
}

void CO_MapDestroy(CO_MAP_T** map)
{
    CO_Assert(map != NULL);
    CO_Assert(*map != NULL);

    CO_MapClear(*map);
    CO_MemFree((*map)->items);

    CO_MemFree(*map);
    *map = NULL;
}

void CO_MapSet(CO_MAP_T* map, uintptr_t key, uintptr_t value)
{
    CO_Assert(map != NULL);

    size_t index = map->hash(key) % map->maxHash;

    CO_MAP_ITEM_T** item = &map->items[index];

    while (*item != NULL)
    {
        if (map->compareKeys((*item)->key, key))
        {
            map->deleteValue((*item)->value);
            (*item)->value = value;
            return;
        }

        if ((*item)->next == NULL)
        {
            item = &(*item)->next;
            break;
        }

        item = &(*item)->next;
    }

    *item = (CO_MAP_ITEM_T*)CO_MemAlloc(sizeof(CO_MAP_ITEM_T));

    (*item)->key = key;
    (*item)->value = value;
    (*item)->next = NULL;
}

bool CO_MapGet(const CO_MAP_T* map, const uintptr_t key, uintptr_t* value)
{
    CO_Assert(map != NULL);

    size_t index = map->hash(key) % map->maxHash;

    const CO_MAP_ITEM_T* item = map->items[index];

    while (item != NULL)
    {
        if (map->compareKeys(item->key, key))
        {
            *value = item->value;
            return true;
        }

        item = item->next;
    }

    return false;
}

void CO_MapRemove(CO_MAP_T* map, const uintptr_t key)
{
    CO_Assert(map != NULL);

    size_t index = map->hash(key) % map->maxHash;

    CO_MAP_ITEM_T* item = map->items[index];
    CO_MAP_ITEM_T* prev = NULL;

    while (item != NULL)
    {
        if (map->compareKeys(item->key, key))
        {
            if (prev != NULL)
            {
                prev->next = item->next;
            }
            else
            {
                map->items[index] = NULL;
            }

            CO_MemFree(item);

            break;
        }

        prev = item;
        item = item->next;
    }
}

bool CO_MapContains(const CO_MAP_T* map, const uintptr_t key)
{
    CO_Assert(map != NULL);

    size_t index = map->hash(key) % map->maxHash;

    const CO_MAP_ITEM_T* item = map->items[index];

    while (item != NULL)
    {
        if (map->compareKeys(item->key, key))
        {
            return true;
        }

        item = item->next;
    }

    return false;
}
