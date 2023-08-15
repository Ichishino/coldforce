#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event_worker.h>

#include <coldforce/net/co_net_thread.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// net thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

bool
co_net_thread_setup(
    co_thread_t* thread,
    co_thread_create_fn create_handler,
    co_thread_destroy_fn destroy_handler
)
{
    if (!co_net_setup())
    {
        return false;
    }

    co_net_worker_t* net_worker = co_net_worker_create();

    net_worker->on_destroy = destroy_handler;

    co_thread_setup_internal(thread,
        create_handler, (co_thread_destroy_fn)co_net_worker_on_destroy,
        (co_event_worker_t*)net_worker);

    return true;
}

void
co_net_thread_cleanup(
    co_thread_t* thread
)
{
    co_net_worker_cleanup(
        (co_net_worker_t*)thread->event_worker);

    co_thread_cleanup(thread);
}

co_net_thread_callbacks_st*
co_net_thread_get_callbacks(
    co_thread_t* thread
)
{
    return &(((co_net_worker_t*)
        thread->event_worker)->callbacks);
}
