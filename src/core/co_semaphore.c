#include <coldforce/core/co_std.h>
#include <coldforce/core/co_semaphore.h>

#ifdef CO_OS_WIN
#   include <windows.h>
#elif defined(CO_OS_LINUX)
#   include <errno.h>
#   include <time.h>
#   include <semaphore.h>
#elif defined(CO_OS_MAC)
#   include <dispatch/dispatch.h>
#endif

//---------------------------------------------------------------------------//
// semaphore
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_semaphore_t*
co_semaphore_create(
    int count
)
{
#ifdef CO_OS_WIN

    HANDLE* semaphore = (HANDLE*)co_mem_alloc(sizeof(HANDLE));

    if (semaphore == NULL)
    {
        return NULL;
    }

    *semaphore = CreateSemaphore(NULL, count, LONG_MAX, NULL);

#elif defined(CO_OS_LINUX)

    sem_t* semaphore = (sem_t*)co_mem_alloc(sizeof(sem_t));

    if (semaphore == NULL)
    {
        return NULL;
    }

    sem_init(semaphore, 0, count);

#elif defined(CO_OS_MAC)

    dispatch_semaphore_t* semaphore = (dispatch_semaphore_t*)
        co_mem_alloc(sizeof(dispatch_semaphore_t));

    if (semaphore == NULL)
    {
        return NULL;
    }

    *semaphore = dispatch_semaphore_create(count);

#else
    void* semaphore = NULL;
#endif

    return (co_semaphore_t*)semaphore;
}

void
co_semaphore_destroy(
    co_semaphore_t* semaphore
)
{
    if (semaphore != NULL)
    {
#ifdef CO_OS_WIN
        CloseHandle(*(HANDLE*)semaphore);
#elif defined(CO_OS_LINUX)
        sem_destroy((sem_t*)semaphore);
#elif defined(CO_OS_MAC)
        dispatch_release(*(dispatch_semaphore_t*)semaphore);
#endif
        co_mem_free(semaphore);
    }
}

co_wait_result_t
co_semaphore_wait(
    co_semaphore_t* semaphore,
    uint32_t msec
)
{
#ifdef CO_OS_WIN

    DWORD result = WaitForSingleObject(*(HANDLE*)semaphore, msec);

    if (result == WAIT_OBJECT_0)
    {
        return CO_WAIT_RESULT_SUCCESS;
    }
    else if (result == WAIT_TIMEOUT)
    {
        return CO_WAIT_RESULT_TIMEOUT;
    }

    return CO_WAIT_RESULT_ERROR;

#elif defined(CO_OS_LINUX)

    int res;

    if (msec == CO_INFINITE)
    {
        do
        {
            res = sem_wait((sem_t*)semaphore);

        } while (res == -1 && errno == EINTR);
    }
    else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_nsec += msec * 1000000;
        ts.tv_sec += ts.tv_nsec / 1000000000;
        ts.tv_nsec %= 1000000000;

        do
        {
            res = sem_timedwait((sem_t*)semaphore, &ts);

        } while (res == -1 && errno == EINTR);
    }

    if (res == 0)
    {
        return CO_WAIT_RESULT_SUCCESS;
    }
    else if (errno == ETIMEDOUT)
    {
        return CO_WAIT_RESULT_TIMEOUT;
    }
    else
    {
        return CO_WAIT_RESULT_ERROR;
    }

#elif defined(CO_OS_MAC)

    dispatch_time_t timeout =
        dispatch_time(DISPATCH_TIME_NOW, msec * NSEC_PER_MSEC);

    return (dispatch_semaphore_wait(
        *(dispatch_semaphore_t*)semaphore, timeout) == 0) ?
        CO_WAIT_RESULT_SUCCESS : CO_WAIT_RESULT_TIMEOUT;

#endif
}

void
co_semaphore_post(
    co_semaphore_t* semaphore
)
{
#ifdef CO_OS_WIN
    ReleaseSemaphore(*(HANDLE*)semaphore, 1, NULL);
#elif defined(CO_OS_LINUX)
    sem_post((sem_t*)semaphore);
#elif defined(CO_OS_MAC)
    dispatch_semaphore_signal(*(dispatch_semaphore_t*)semaphore);
#endif
}
