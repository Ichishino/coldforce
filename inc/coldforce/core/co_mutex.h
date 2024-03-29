#ifndef CO_MUTEX_H_INCLUDED
#define CO_MUTEX_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// mutex
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    uintptr_t unused;

} co_mutex_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_CORE_API
co_mutex_t*
co_mutex_create(
    void
);

CO_CORE_API
void
co_mutex_destroy(
    co_mutex_t* mutex
);

CO_CORE_API
void
co_mutex_lock(
    co_mutex_t* mutex
);

CO_CORE_API
void
co_mutex_unlock(
    co_mutex_t* mutex
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_MUTEX_H_INCLUDED
