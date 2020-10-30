#include <coldforce/core/co_std.h>
#include <coldforce/core/co_mutex.h>

#ifdef CO_OS_WIN
#   include <windows.h>
#else
#   include <pthread.h>
#endif

//---------------------------------------------------------------------------//
// mutex
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_mutex_t*
co_mutex_create(
    void
)
{
#ifdef CO_OS_WIN
    CRITICAL_SECTION* mutex =
        (CRITICAL_SECTION*)co_mem_alloc(sizeof(CRITICAL_SECTION));

    if (mutex == NULL)
    {
        return NULL;
    }

    InitializeCriticalSection(mutex);
#else
    pthread_mutex_t* mutex =
        (pthread_mutex_t*)co_mem_alloc(sizeof(pthread_mutex_t));

    if (mutex == NULL)
    {
        return NULL;
    }

    pthread_mutex_init(mutex, NULL);
#endif

    return (co_mutex_t*)mutex;
}

void
co_mutex_destroy(
    co_mutex_t* mutex
)
{
    if (mutex != NULL)
    {
#ifdef CO_OS_WIN
        DeleteCriticalSection((CRITICAL_SECTION*)mutex);
#else
        pthread_mutex_destroy((pthread_mutex_t*)mutex);
#endif
        co_mem_free(mutex);
    }
}

void
co_mutex_lock(
    co_mutex_t* mutex
)
{
#ifdef CO_OS_WIN
    EnterCriticalSection((CRITICAL_SECTION*)mutex);
#else
    pthread_mutex_lock((pthread_mutex_t*)mutex);
#endif
}

void
co_mutex_unlock(
    co_mutex_t* mutex
)
{
#ifdef CO_OS_WIN
    LeaveCriticalSection((CRITICAL_SECTION*)mutex);
#else
    pthread_mutex_unlock((pthread_mutex_t*)mutex);
#endif
}
