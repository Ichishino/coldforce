#include <coldforce/co_sem.h>

#ifdef CO_OS_WIN
#include <windows.h>
#elif defined(CO_OS_LINUX)
#include <errno.h>
#include <time.h>
#elif defined(CO_OS_MAC)
#endif

//---------------------------------------------------------------------------//
// Semaphore
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_SEM_T* CO_SemCreate(int count)
{
    CO_SEM_T* sem = (CO_SEM_T*)CO_MemAlloc(sizeof(CO_SEM_T));

#ifdef CO_OS_WIN
    sem->handle = CreateSemaphore(NULL, count, LONG_MAX, NULL);
#elif defined(CO_OS_LINUX)
    sem_init(&sem->handle, 0, count);
#elif defined(CO_OS_MAC)
    sem->handle = dispatch_semaphore_create(count);
#else
    sem = NULL;
#endif

    return sem;
}

void CO_SemDestroy(CO_SEM_T** sem)
{
    CO_Assert(sem != NULL);
    CO_Assert(*sem != NULL);

#ifdef CO_OS_WIN
    CloseHandle((*sem)->handle);
#elif defined(CO_OS_LINUX)
    sem_destroy(&(*sem)->handle);
#elif defined(CO_OS_MAC)
    dispatch_release((*sem)->handle);
#endif

    CO_MemFree(*sem);
    *sem = NULL;
}

CO_WAIT_RESULT_EN CO_SemWait(CO_SEM_T* sem, uint32_t msec)
{
    CO_Assert(sem != NULL);

#ifdef CO_OS_WIN

    DWORD result = WaitForSingleObject(sem->handle, msec);

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

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_nsec += msec * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    int res;
    do
    {
        res = sem_timedwait(&sem->handle, &ts);
    
    } while (res == -1 && errno == EINTR);

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

    return (dispatch_semaphore_wait(sem->handle, timeout) == 0) ?
        CO_WAIT_RESULT_SUCCESS : CO_WAIT_RESULT_TIMEOUT;

#endif
}

void CO_SemPost(CO_SEM_T* sem)
{
    CO_Assert(sem != NULL);

#ifdef CO_OS_WIN
    ReleaseSemaphore(sem->handle, 1, NULL);
#elif defined(CO_OS_LINUX)
    sem_post(&sem->handle);
#elif defined(CO_OS_MAC)
    dispatch_semaphore_signal(sem->handle);
#endif
}
