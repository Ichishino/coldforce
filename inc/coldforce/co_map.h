#ifndef _COLDFORCE_MAP_H_
#define _COLDFORCE_MAP_H_

#include <coldforce/co_std.h>

//---------------------------------------------------------------------------//
// Map
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef size_t(*CO_MAP_HASH_FN)(const uintptr_t);
typedef void(*CO_MAP_DELETE_FN)(uintptr_t);
typedef bool(*CO_MAP_COMPARE_FN)(const uintptr_t, const uintptr_t);

typedef struct
{
    size_t maxHash;

    CO_MAP_HASH_FN hash;
    CO_MAP_COMPARE_FN compareKeys;
    CO_MAP_DELETE_FN deleteKey;
    CO_MAP_DELETE_FN deleteValue;

} CO_MAP_PARAM_ST;

struct CO_MAP_ITEM_ST
{
    uintptr_t key;
    uintptr_t value;

    struct CO_MAP_ITEM_ST* next;
};
typedef struct CO_MAP_ITEM_ST CO_MAP_ITEM_T;

typedef struct
{
    size_t maxHash;
    CO_MAP_ITEM_T** items;

    CO_MAP_HASH_FN hash;
    CO_MAP_COMPARE_FN compareKeys;
    CO_MAP_DELETE_FN deleteKey;
    CO_MAP_DELETE_FN deleteValue;

} CO_MAP_T;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_API CO_MAP_T* CO_MapCreate(const CO_MAP_PARAM_ST* param);
CO_API void CO_MapDestroy(CO_MAP_T** map);
CO_API void CO_MapClear(CO_MAP_T* map);
CO_API void CO_MapSet(CO_MAP_T* map, uintptr_t key, uintptr_t value);
CO_API bool CO_MapGet(const CO_MAP_T* map, const uintptr_t key, uintptr_t* value);
CO_API void CO_MapRemove(CO_MAP_T* map, const uintptr_t key);
CO_API bool CO_MapContains(const CO_MAP_T* map, const uintptr_t key);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // _COLDFORCE_MAP_H_
