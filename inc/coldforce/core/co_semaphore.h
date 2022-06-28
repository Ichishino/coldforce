#ifndef CO_SEMAPHORE_H_INCLUDED
#define CO_SEMAPHORE_H_INCLUDED

#include <coldforce/core/co.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// semaphore
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    uintptr_t unused;

} co_semaphore_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_API
co_semaphore_t*
co_semaphore_create(
    int count
);

CO_API
void
co_semaphore_destroy(
    co_semaphore_t* semaphore
);

CO_API
co_wait_result_t
co_semaphore_wait(
    co_semaphore_t* semaphore,
    uint32_t msec
);

CO_API
void
co_semaphore_post(
    co_semaphore_t* semaphore
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SEMAPHORE_H_INCLUDED
