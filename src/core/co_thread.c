#include <coldforce/core/co_std.h>
#include <coldforce/core/co_thread.h>
#include <coldforce/core/co_string.h>
#include <coldforce/core/co_log.h>

#ifdef CO_OS_WIN
#   include <windows.h>
#   include <process.h>
#else
#   include <pthread.h>
#   include <sys/syscall.h>
#   include <sys/types.h>
#   include <errno.h>
#endif

//---------------------------------------------------------------------------//
// thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

CO_THREAD_LOCAL co_thread_t* current_thread = NULL;

struct co_thread_param_st
{
    co_thread_t* thread;
    co_semaphore_t* semaphore;
    bool create_result;
};

static
#ifdef CO_OS_WIN
unsigned int WINAPI
#else
void*
#endif
co_thread_main(
    void* param
)
{
    struct co_thread_param_st* thread_param =
        (struct co_thread_param_st*)param;
    co_thread_t* thread = thread_param->thread;

    co_assert(current_thread == NULL);
    current_thread = thread;

#ifdef CO_OS_WIN
    thread->id = GetCurrentThreadId();
#else
    thread->id = syscall(SYS_gettid);
#endif
    co_core_log_info(
        "thread [%08lx] start", thread->id);

    bool create_result = true;

    if (thread->on_create != NULL)
    {
        create_result = thread->on_create(thread);

        if (!create_result)
        {
            thread->exit_code = -1;
        }
    }

    thread_param->create_result = create_result;
    co_semaphore_post(thread_param->semaphore);

    if (create_result)
    {
        co_thread_run(thread);
    }

    if (thread->on_destroy != NULL)
    {
        thread->on_destroy(thread);
    }

    co_core_log_info(
        "thread [%08lx] exit: (%d)", thread->id, thread->exit_code);

#ifdef CO_OS_WIN
    return thread->exit_code;
#else
    return (void*)(uintptr_t)thread->exit_code;
#endif
}

void
co_thread_setup_internal(
    co_thread_t* thread,
    const char* name,
    co_thread_create_fn create_handler,
    co_thread_destroy_fn destroy_handler,
    co_event_worker_t* event_worker
)
{
    thread->id = 0;
    thread->handle = NULL;
    thread->parent = NULL;

    thread->on_create = create_handler;
    thread->on_destroy = destroy_handler;
    thread->event_worker = event_worker;

    if (name != NULL)
    {
        thread->name = co_string_duplicate(name);
    }

    if (thread->event_worker == NULL)
    {
        thread->event_worker = co_event_worker_create();
    }

    co_event_worker_setup(thread->event_worker);

    thread->exit_code = 0;
}

void
co_thread_run(
    co_thread_t* thread
)
{
    co_event_worker_run(thread->event_worker);
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_thread_setup(
    co_thread_t* thread,
    const char* name,
    co_thread_create_fn create_handler,
    co_thread_destroy_fn destroy_handler
)
{
    co_thread_setup_internal(
        thread, name, create_handler, destroy_handler, NULL);
}

void
co_thread_cleanup(
    co_thread_t* thread
)
{
    if (thread->event_worker != NULL)
    {
        co_event_worker_cleanup(thread->event_worker);
        co_event_worker_destroy(thread->event_worker);
     
        thread->event_worker = NULL;
    }

    if (thread->name != NULL)
    {
        co_string_destroy(thread->name);
        thread->name = NULL;
    }

    if (thread->handle != NULL)
    {
#ifdef CO_OS_WIN
        CloseHandle((HANDLE)thread->handle);
#else
        co_mem_free(thread->handle);
#endif
        thread->handle = NULL;
    }
}

co_thread_t*
co_thread_get_current(
    void
)
{
    return current_thread;
}

bool
co_thread_start(
    co_thread_t* thread
)
{
    thread->parent = co_thread_get_current();

    struct co_thread_param_st thread_param;
    thread_param.thread = thread;
    thread_param.semaphore = co_semaphore_create(0);

#ifdef CO_OS_WIN
    thread->handle = (HANDLE)_beginthreadex(
        NULL, 0, co_thread_main, &thread_param, 0, NULL);
#else
    pthread_t* pthread = (pthread_t*)co_mem_alloc(sizeof(pthread_t));
    memset(pthread, 0x00, sizeof(pthread_t));

    if (pthread_create(pthread, NULL, co_thread_main, &thread_param) != 0)
    {
        co_core_log_error("pthread_create error: (%d)", errno);
        co_mem_free(pthread);
        co_semaphore_destroy(thread_param.semaphore);

        return false;
    }

    thread->handle = (co_thread_handle_t*)pthread;
#endif

    if (thread->handle == NULL)
    {
        co_semaphore_destroy(thread_param.semaphore);

        return false;
    }

    co_semaphore_wait(thread_param.semaphore, CO_INFINITE);
    co_semaphore_destroy(thread_param.semaphore);

    if (!thread_param.create_result)
    {
        co_thread_join(thread);

        return false;
    }

    return true;
}

void
co_thread_stop(
    co_thread_t* thread
)
{
    if ((thread != NULL) &&
        (thread->event_worker != NULL))
    {
        co_thread_send_event(thread, CO_EVENT_ID_STOP, 0, 0);
    }
}

void
co_thread_join(
    co_thread_t* thread
)
{
    if ((thread != NULL) &&
        (thread->handle != NULL))
    {
#ifdef CO_OS_WIN
        WaitForSingleObject((HANDLE)thread->handle, INFINITE);
#else
        pthread_join(*((pthread_t*)thread->handle), NULL);
#endif
    }
}

void
co_thread_set_exit_code(
    int exit_code
)
{
    co_thread_t* thread = co_thread_get_current();

    thread->exit_code = exit_code;
}

int
co_thread_get_exit_code(
    const co_thread_t* thread
)
{
    return thread->exit_code;
}

co_thread_t*
co_thread_get_parent(
    co_thread_t* thread
)
{
    return thread->parent;
}

co_thread_id_t
co_thread_get_id(
    const co_thread_t* thread
)
{
    return thread->id;
}

co_thread_handle_t*
co_thread_get_handle(
    co_thread_t* thread
)
{
    return thread->handle;
}

const char*
co_thread_get_name(
    const co_thread_t* thread
)
{
    return thread->name;
}
