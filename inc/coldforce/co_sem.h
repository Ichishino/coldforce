#ifndef _COLDFORCE_SEM_H_
#define _COLDFORCE_SEM_H_

#include <coldforce/co_std.h>

#ifdef CO_OS_WIN
#elif defined(CO_OS_MAC)
#include <dispatch/dispatch.h>
#elif defined(CO_OS_LINUX)
#include <semaphore.h>
#endif

//---------------------------------------------------------------------------//
// Semaphore
//---------------------------------------------------------------------------//

typedef struct
{
#ifdef CO_OS_WIN
    CO_WIN_HANDLE_T handle;
#elif defined(CO_OS_LINUX)
    sem_t handle;
#elif defined(CO_OS_MAC)

#endif

} CO_SEM_T;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_API CO_SEM_T* CO_SemCreate(int count);
CO_API void CO_SemDestroy(CO_SEM_T** sem);

CO_API CO_WAIT_RESULT_EN CO_SemWait(CO_SEM_T* sem, uint32_t msec);
CO_API void CO_SemPost(CO_SEM_T* sem);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // _COLDFORCE_SEM_H_
