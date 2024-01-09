#include "test_app.h"

//---------------------------------------------------------------------------//
// test_app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static test_thread_st*
test_tcp_1(
    void
)
{
    size_t thread_size = sizeof(test_tcp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test tcp (ipv4)";
    thread->ctx.run = test_tcp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_IPV4;
    thread->ctx.server_address = "127.0.0.1";
    thread->ctx.server_port = 9001;
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_tcp_2(
    void
)
{
    size_t thread_size = sizeof(test_tcp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test tcp (ipv6)";
    thread->ctx.run = test_tcp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_IPV6;
    thread->ctx.server_address = "::1";
    thread->ctx.server_port = 9002;
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_tcp_3(
    void
)
{
    size_t thread_size = sizeof(test_tcp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test tcp (unix)";
    thread->ctx.run = test_tcp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_UNIX;
    thread->ctx.server_address = "./test_tcp_server.sock";
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_tcp_4(
    void
)
{
    size_t thread_size = sizeof(test_tcp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test tcp (mt)";
    thread->ctx.run = test_tcp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_IPV4;
    thread->ctx.server_address = "127.0.0.1";
    thread->ctx.server_port = 9004;
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_udp_1(
    void
)
{
    size_t thread_size = sizeof(test_udp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test udp (ipv4)";
    thread->ctx.run = test_udp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_IPV4;
    thread->ctx.server_address = "127.0.0.1";
    thread->ctx.server_port = 9101;
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_udp_2(
    void
)
{
    size_t thread_size = sizeof(test_udp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test udp (ipv6)";
    thread->ctx.run = test_udp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_IPV6;
    thread->ctx.server_address = "::1";
    thread->ctx.server_port = 9102;
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_udp_3(
    void
)
{
    size_t thread_size = sizeof(test_udp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test udp (unix)";
    thread->ctx.run = test_udp_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_UNIX;
    thread->ctx.server_address = "./test_udp_server.sock";
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static test_thread_st*
test_udp2_1(
    void
)
{
    size_t thread_size = sizeof(test_udp_thread_st);

    test_thread_st* thread =
        (test_thread_st*)co_mem_alloc(thread_size);

    memset(thread, 0x00, thread_size);

    thread->ctx.title = "test udp2 (ipv4)";
    thread->ctx.run = test_udp2_run;

    thread->ctx.family = CO_NET_ADDR_FAMILY_IPV4;
    thread->ctx.server_address = "127.0.0.1";
    thread->ctx.server_port = 9201;
    thread->ctx.data_size = 100000;
    thread->ctx.client_count = 100;
    thread->ctx.time_limit_sec = 5 * 60;

    return thread;
}

static void
test_app_init_items(
    test_app_st* self
)
{
    size_t index = 0;

    self->threads[index++] = test_tcp_1();
    self->threads[index++] = test_tcp_2();
#ifndef _WIN32
    self->threads[index++] = test_tcp_3();
#endif
//    self->threads[index++] = test_tcp_4();

    self->threads[index++] = test_udp_1();
    self->threads[index++] = test_udp_2();
#ifndef _WIN32
    self->threads[index++] = test_udp_3();
//    self->threads[index++] = test_udp2_1();
#endif

    self->threads[index++] = NULL;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static void
test_app_on_timer(
    test_app_st* self,
    co_timer_t* timer
)
{
    (void)timer;

    test_error("**** test(%d) timeout ****", self->item_index + 1);

    exit(-1);
}

static void
test_app_item_start(
    test_app_st* self
)
{
    test_thread_st* thread =
        self->threads[self->item_index];

    test_info("<<== test(%d) %s start ==>>",
        self->item_index + 1, thread->ctx.title);

    uint32_t time_limit_sec =
        thread->ctx.time_limit_sec;

    if (self->timer == NULL)
    {
        self->timer = co_timer_create(time_limit_sec * 1000,
            (co_timer_fn)test_app_on_timer, false, NULL);
    }
    else
    {
        co_timer_set_time(self->timer, time_limit_sec * 1000);
    }

    self->start_time = co_get_current_time_in_msec();

    co_timer_start(self->timer);

    thread->ctx.run((co_thread_t*)thread);
}

static void
test_app_on_item_finished(
    test_app_st* self,
    const co_event_st* event
)
{
    (void)event;

    uint64_t msec =
        co_get_current_time_in_msec() - self->start_time;
    self->total_time += msec;

    test_info("<<== test(%d) finished: (%.4f sec) ==>>",
        self->item_index + 1, (float)msec / 1000.0f);

    co_timer_stop(self->timer);

    co_thread_t* thread =
        (co_thread_t*)self->threads[self->item_index];

    if (thread != NULL)
    {
        co_thread_join(thread);
        co_net_thread_cleanup(thread);
        co_mem_free(thread);

        self->threads[self->item_index] = NULL;
    }

    self->item_index++;

    if (self->threads[self->item_index] == NULL)
    {
        test_info("<<== all finished - total time: (%.4f sec) ==>>",
            (float)self->total_time / 1000.0f);

        co_app_stop();

        return;
    }

    test_app_item_start(self);
}

static bool
test_app_on_create(
    test_app_st* self
)
{
    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_ID_TEST_FINISHED,
        (co_event_fn)test_app_on_item_finished);

    self->total_time = 0;
    self->item_index = 0;

    test_app_init_items(self);
    test_app_item_start(self);

    return true;
}

static void
test_app_on_destroy(
    test_app_st* self
)
{
    co_timer_destroy(self->timer);
    self->timer = NULL;

    int app_exit_code = 0;

    for (size_t index = 0;
        index < TEST_THREAD_COUNT;
        index++)
    {
        co_thread_t* thread =
            (co_thread_t*)self->threads[index];

        if (thread != NULL)
        {
            co_thread_stop(thread);
            co_thread_join(thread);

            int exit_code = co_thread_get_exit_code(thread);

            if (app_exit_code == 0 && exit_code != 0)
            {
                app_exit_code = exit_code;
            }

            co_net_thread_cleanup(thread);
            co_mem_free(thread);
        }
    }

    co_app_set_exit_code(app_exit_code);
}

//---------------------------------------------------------------------------//
// test_app run
//---------------------------------------------------------------------------//

int
test_app_run(
    int argc,
    char** argv
)
{
    co_tls_debug_mem_check();

    co_core_log_set_level(CO_LOG_LEVEL_MAX);
    co_tcp_log_set_level(CO_LOG_LEVEL_WARNING);
    co_udp_log_set_level(CO_LOG_LEVEL_WARNING);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);

    co_log_add_category(TEST_LOG_CATEGORY, "TEST");
    co_log_set_level(TEST_LOG_CATEGORY, CO_LOG_LEVEL_MAX);

    co_tls_setup();

    test_app_st test_app = { 0 };

    int exit_code =
        co_net_app_start(
            (co_app_t*)&test_app, "test_app",
            (co_app_create_fn)test_app_on_create,
            (co_app_destroy_fn)test_app_on_destroy,
            argc, argv);

    co_tls_cleanup();

    return exit_code;
}
