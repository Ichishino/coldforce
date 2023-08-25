#include <coldforce/core/co_std.h>
#include <coldforce/core/co_app.h>
#include <coldforce/core/co_log.h>

#ifdef CO_OS_WIN
#   include <windows.h>
#else
#   include <pthread.h>
#endif

//---------------------------------------------------------------------------//
// app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static co_app_t* current_app = NULL;
extern CO_THREAD_LOCAL co_thread_t* current_thread;

void
co_app_setup_internal(
    co_app_t* app,
    co_app_create_fn create_handler,
    co_app_destroy_fn destroy_handler,
    co_event_worker_t* event_worker,
    int argc,
    char** argv
)
{
    co_assert(current_app == NULL);
    co_assert(current_thread == NULL);

    co_thread_setup_internal(&app->main_thread,
        create_handler, destroy_handler, event_worker);

#ifdef CO_OS_WIN
    app->main_thread.handle =
        (co_thread_handle_t*)GetCurrentThread();
#else
    pthread_t* pthread = (pthread_t*)co_mem_alloc(sizeof(pthread_t));
    *pthread = pthread_self();
    app->main_thread.handle = (co_thread_handle_t*)pthread;
#endif

    app->args.count = argc;
    app->args.values = argv;

    current_app = app;
    current_thread = (co_thread_t*)app;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_app_setup(
    co_app_t* app,
    co_app_create_fn create_handler,
    co_app_destroy_fn destroy_handler,
    int argc,
    char** argv
)
{
    co_app_setup_internal(
        app, create_handler, destroy_handler, NULL,
        argc, argv);
}

void
co_app_cleanup(
    co_app_t* app
)
{
    if (app != NULL)
    {
        co_thread_cleanup(&app->main_thread);
    }
}

int
co_app_run(
    co_app_t* app
)
{
    co_core_log_info("app start");

    bool create_result = true;

    if (app->main_thread.on_create != NULL)
    {
        create_result =
            app->main_thread.on_create((co_thread_t*)app);
    }

    if (create_result)
    {
        co_thread_run((co_thread_t*)app);
    }

    if (app->main_thread.on_destroy != NULL)
    {
        app->main_thread.on_destroy((co_thread_t*)app);
    }

    co_core_log_info(
        "app exit: %d", app->main_thread.exit_code);

    return app->main_thread.exit_code;
}

void
co_app_stop(
    void
)
{
    co_app_t* app = co_app_get_current();

    if (app != NULL)
    {
        co_thread_stop(&app->main_thread);
    }
}

const co_args_st*
co_app_get_args(
    const co_app_t* app
)
{
    return &app->args;
}

co_app_t*
co_app_get_current(
    void
)
{
    return current_app;
}

void
co_app_set_exit_code(
    int exit_code
)
{
    co_thread_set_exit_code(exit_code);
}

int
co_app_get_exit_code(
    void
)
{
    return co_thread_get_exit_code((co_thread_t*)current_app);
}
