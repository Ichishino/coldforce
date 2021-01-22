#include <coldforce/core/co_std.h>
#include <coldforce/core/co_event_worker.h>

#include <coldforce/net/co_net_thread.h>
#include <coldforce/net/co_net_worker.h>

//---------------------------------------------------------------------------//
// net thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_net_thread_init(
    co_thread_t* thread,
    co_thread_create_fn create_handler,
    co_thread_destroy_fn destroy_handler
)
{
    co_net_worker_t* net_worker = co_net_worker_create();

    net_worker->on_destroy = destroy_handler;

    co_thread_setup(thread,
        create_handler, (co_thread_destroy_fn)co_net_worker_on_destroy,
        (co_event_worker_t*)net_worker);
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

bool
co_net_thread_start(
    co_thread_t* thread,
    uintptr_t param
)
{
    return co_thread_start(thread, param);
}

void
co_net_thread_stop(
    co_thread_t* thread
)
{
    co_thread_stop(thread);
}

void
co_net_thread_wait(
    co_thread_t* thread
)
{
    co_thread_wait(thread);
}
