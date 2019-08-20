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

CO_MTX_T* CO_MtxCreate()
{
    CO_MTX_T* mtx = (CO_MTX_T*)CO_MemAlloc(sizeof(CO_MTX_T));

#ifdef CO_OS_WIN
    InitializeCriticalSection(&mtx->handle);
#elif defined(CO_OS_LINUX)
    pthread_mutex_init(&mtx->handle, NULL);
#elif defined(CO_OS_MAC)
#endif
    return mtx;
}

void CO_MtxDestroy(CO_MTX_T** mtx)
{
    CO_Assert(mtx != NULL);
    CO_Assert(*mtx != NULL);

#ifdef CO_OS_WIN
    DeleteCriticalSection(&(*mtx)->handle);
#elif defined(CO_OS_LINUX)
    pthread_mutex_destroy(&(*mtx)->handle);
#elif defined(CO_OS_MAC)
#endif

    CO_MemFree(*mtx);
    *mtx = NULL;
}

void CO_MtxLock(CO_MTX_T* mtx)
{
    CO_Assert(mtx != NULL);

#ifdef CO_OS_WIN
    EnterCriticalSection(&mtx->handle);
#elif defined(CO_OS_LINUX)
    pthread_mutex_lock(&mtx->handle);
#elif defined(CO_OS_MAC)
#endif
}

void CO_MtxUnlock(CO_MTX_T* mtx)
{
    CO_Assert(mtx != NULL);

#ifdef CO_OS_WIN
    LeaveCriticalSection(&mtx->handle);
#elif defined(CO_OS_LINUX)
    pthread_mutex_unlock(&mtx->handle);
#elif defined(CO_OS_MAC)
#endif
}
