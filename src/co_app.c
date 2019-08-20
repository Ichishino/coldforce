#include <coldforce/co_app.h>

#ifdef CO_OS_WIN
#include <windows.h>
#else
#include <pthread.h>
#endif

//---------------------------------------------------------------------------//
// App
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

static CO_APP_T* gApp = NULL;
extern CO_THREAD_LOCAL CO_THREAD_T* gThread;

void CO_AppInit(CO_APP_T* app, const CO_CTX_ST* ctx)
{
    CO_Assert(gApp == NULL);
    CO_Assert(gThread == NULL);

    CO_ThreadInit(&app->th, ctx);

#ifdef CO_OS_WIN
    app->th.handle = GetCurrentThread();
#else
    app->th.handle = pthread_self();
#endif

    gApp = app;
    gThread = (CO_THREAD_T*)app;
}

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_APP_T* CO_AppGetCurrent()
{
    return gApp;
}

CO_APP_T* CO_AppCreate(const CO_CTX_ST* ctx)
{
    size_t appLen = sizeof(CO_APP_T);

    if (ctx != NULL)
    {
        appLen = ctx->objLen;
    }

    if (appLen < sizeof(CO_APP_T))
    {
        appLen = sizeof(CO_APP_T);
    }

    CO_APP_T* app = (CO_APP_T*)CO_MemAlloc(appLen);

    CO_AppInit(app, ctx);

    return app;
}

void CO_AppDestroy(CO_APP_T** app)
{
    CO_Assert(app != NULL);
    CO_Assert(*app != NULL);
    CO_Assert(gApp != NULL);

    CO_ThreadClear(&(*app)->th);

    CO_MemFree(*app);
    *app = NULL;

    gThread = NULL;
    gApp = NULL;
}

int CO_AppRun(CO_APP_T* app, CO_APP_PARAM_ST* param)
{
    CO_Assert(app != NULL);
    CO_Assert(gApp != NULL);

    bool createResult = true;

    if (app->th.onCreate != NULL)
    {
        createResult = app->th.onCreate(app, (uintptr_t)param);
    }

    if (createResult)
    {
        CO_EvLoopRun((CO_EVENT_LOOP_T*)app);
    }

    if (app->th.onDestroy != NULL)
    {
        app->th.onDestroy(app);
    }

    return app->th.exitCode;
}

int CO_AppStart(const CO_CTX_ST* ctx, CO_APP_PARAM_ST* param)
{
    CO_APP_T* app = CO_AppCreate(ctx);

    int exitCode = CO_AppRun(app, param);

    CO_AppDestroy(&app);

    return exitCode;
}

void CO_AppStop()
{
    CO_APP_T* app = CO_AppGetCurrent();
    CO_Assert(app != NULL);

    CO_ThreadStop((CO_THREAD_T*)app);
}
