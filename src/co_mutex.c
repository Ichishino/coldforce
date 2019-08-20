#include <coldforce/co_mutex.h>

#ifdef CO_OS_WIN
#include <windows.h>
#endif

//---------------------------------------------------------------------------//
// Mutex
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_MTX_T* CO_MtxCreate(void)
{
    CO_MTX_T* mtx = (CO_MTX_T*)CO_MemAlloc(sizeof(CO_MTX_T));

#ifdef CO_OS_WIN
    InitializeCriticalSection(&mtx->handle);
#else
    pthread_mutex_init(&mtx->handle, NULL);
#endif

    return mtx;
}

void CO_MtxDestroy(CO_MTX_T** mtx)
{
    CO_Assert(mtx != NULL);
    CO_Assert(*mtx != NULL);

#ifdef CO_OS_WIN
    DeleteCriticalSection(&(*mtx)->handle);
#else
    pthread_mutex_destroy(&(*mtx)->handle);
#endif

    CO_MemFree(*mtx);
    *mtx = NULL;
}

void CO_MtxLock(CO_MTX_T* mtx)
{
    CO_Assert(mtx != NULL);

#ifdef CO_OS_WIN
    EnterCriticalSection(&mtx->handle);
#else
    pthread_mutex_lock(&mtx->handle);
#endif
}

void CO_MtxUnlock(CO_MTX_T* mtx)
{
    CO_Assert(mtx != NULL);

#ifdef CO_OS_WIN
    LeaveCriticalSection(&mtx->handle);
#else
    pthread_mutex_unlock(&mtx->handle);
#endif
}
