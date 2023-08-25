#include <coldforce/core/co_std.h>
#include <coldforce/core/co_app.h>
#include <coldforce/core/co_event_worker.h>

#include <coldforce/net/co_net_app.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// net app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static bool
co_net_app_setup(
    co_app_t* app,
    co_app_create_fn create_handler,
    co_app_destroy_fn destroy_handler,
    int argc,
    char** argv
)
{
    if (!co_net_setup())
    {
        return false;
    }

    co_net_worker_t* net_worker = co_net_worker_create();

    net_worker->on_destroy = destroy_handler;

    co_app_setup_internal(app,
        create_handler, (co_app_destroy_fn)co_net_worker_on_destroy,
        (co_event_worker_t*)net_worker,
        argc, argv);

    return true;
}

static void
co_net_app_cleanup(
    co_app_t* app
)
{
    if (app != NULL)
    {
        co_net_worker_cleanup(
            (co_net_worker_t*)app->main_thread.event_worker);

        co_app_cleanup(app);
        co_net_cleanup();
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

int
co_net_app_start(
    co_app_t* app,
    co_app_create_fn create_handler,
    co_app_destroy_fn destroy_handler,
    int argc,
    char** argv
)
{
    if (!co_net_app_setup(
        app,
        create_handler, destroy_handler,
        argc, argv))
    {
        return -1;
    }

    int exit_code = co_app_run(app);

    co_net_app_cleanup(app);

    return exit_code;
}
