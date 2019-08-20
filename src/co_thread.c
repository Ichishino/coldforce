#include <coldforce/co_thread.h>
#include <coldforce/co_sem.h>

#ifdef CO_OS_WIN
#include <windows.h>
#include <process.h>
#endif

//---------------------------------------------------------------------------//
// Thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

CO_THREAD_LOCAL CO_THREAD_T* gThread = NULL;

CO_THREAD_T* CO_ThreadCreate(const CO_CTX_ST* ctx)
{
    size_t threadLen = sizeof(CO_THREAD_T);

    if (ctx != NULL)
    {
        threadLen = ctx->objLen;
    }

    if (threadLen < sizeof(CO_THREAD_T))
    {
        threadLen = sizeof(CO_THREAD_T);
    }

    CO_THREAD_T* thread = (CO_THREAD_T*)CO_MemAlloc(threadLen);

    CO_ThreadInit(thread, ctx);

    return thread;
}

void CO_ThreadInit(CO_THREAD_T* thread, const CO_CTX_ST* ctx)
{
    CO_Assert(thread != NULL);

    thread->handle = (CO_THREAD_HANDLE_T)NULL;

    if (ctx != NULL)
    {
        thread->onCreate = ctx->onCreate;
        thread->onDestroy = ctx->onDestroy;
    }
    else
    {
        thread->onCreate = NULL;
        thread->onDestroy = NULL;
    }

    CO_EvLoopInit(&thread->eloop, 8);

    thread->exitCode = 0;
}

void CO_ThreadClear(CO_THREAD_T* thread)
{
    CO_Assert(thread != NULL);

    thread->exitCode = 0;

    CO_EvLoopClear(&thread->eloop);

#ifdef CO_OS_WIN
    CloseHandle(thread->handle);
#endif
    thread->handle = (CO_THREAD_HANDLE_T)NULL;
}

struct CO_ThreadParam
{
    CO_THREAD_T* thread;
    CO_SEM_T* sem;
    uintptr_t param;
    bool createResult;
};

#ifdef CO_OS_WIN
unsigned int WINAPI CO_ThreadMain(void* param)
#else
void* CO_ThreadMain(void* param)
#endif
{
    struct CO_ThreadParam* threadParam =
        (struct CO_ThreadParam*)param;
    CO_THREAD_T* thread = threadParam->thread;

    CO_Assert(thread != NULL);
    CO_Assert(gThread == NULL);

    gThread = thread;

    bool createResult = true;

    if (thread->onCreate != NULL)
    {
        createResult = thread->onCreate(thread, threadParam->param);

        if (!createResult)
        {
            thread->exitCode = -1;
        }
    }

    threadParam->createResult = createResult;
    CO_SemPost(threadParam->sem);

    if (createResult)
    {
        CO_EvLoopRun((CO_EVENT_LOOP_T*)thread);
    }

    if (thread->onDestroy != NULL)
    {
        thread->onDestroy(thread);
    }

#ifdef CO_OS_WIN
    return thread->exitCode;
#elif defined (CO_OS_LINUX)
    return NULL;
#elif defined (CO_OS_MAC)
#endif
}

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_THREAD_T* CO_ThreadGetCurrent()
{
    return gThread;
}

void CO_ThreadDestroy(CO_THREAD_T** thread)
{
    CO_Assert(thread != NULL);
    CO_Assert(*thread != NULL);

    CO_ThreadClear(*thread);

    CO_MemFree(*thread);
    *thread = NULL;
}

CO_THREAD_T* CO_ThreadStart(const CO_CTX_ST* ctx, uintptr_t param)
{
    CO_THREAD_T* thread = CO_ThreadCreate(ctx);

    struct CO_ThreadParam threadParam;
    threadParam.thread = thread;
    threadParam.sem = CO_SemCreate(0);
    threadParam.param = param;

#ifdef CO_OS_WIN
    thread->handle = (CO_WIN_HANDLE_T)_beginthreadex(
        NULL, 0, CO_ThreadMain, &threadParam, 0, NULL);
#else
    pthread_create(&thread->handle, NULL, CO_ThreadMain, &threadParam);
#endif

    if (thread->handle == (CO_THREAD_HANDLE_T)NULL)
    {
        CO_ThreadDestroy(&thread);
        return NULL;
    }

    CO_SemWait(threadParam.sem, CO_INFINITE);
    CO_SemDestroy(&threadParam.sem);

    if (!threadParam.createResult)
    {
        CO_ThreadWait(thread);
        CO_ThreadDestroy(&thread);

        return NULL;
    }

    return thread;
}

void CO_ThreadStop(CO_THREAD_T* thread)
{
    CO_Assert(thread != NULL);

    CO_EventPost(thread, CO_EVENT_ID_STOP, 0);
}

void CO_ThreadWait(CO_THREAD_T* thread)
{
    CO_Assert(thread != NULL);

#ifdef CO_OS_WIN
    WaitForSingleObject(thread->handle, INFINITE);
#else
    pthread_join(thread->handle, NULL);
#endif
}
