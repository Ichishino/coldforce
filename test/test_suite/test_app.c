#include "test_app.h"
#include "test_tcp.h"

//---------------------------------------------------------------------------//
// test_app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static void test_tcp_1(test_app_st* self)
{
    test_info("test tcp (ipv4)");

    self->test_tcp_1_thread.family = CO_NET_ADDR_FAMILY_IPV4;
    self->test_tcp_1_thread.server_address = "127.0.0.1";
    self->test_tcp_1_thread.server_port = 9001;
    self->test_tcp_1_thread.data_size = 100000;
    self->test_tcp_1_thread.client_count = 100;

    test_tcp_run(&self->test_tcp_1_thread);
}

static void test_tcp_2(test_app_st* self)
{
    test_info("test tcp (ipv6)");

    self->test_tcp_2_thread.family = CO_NET_ADDR_FAMILY_IPV6;
    self->test_tcp_2_thread.server_address = "::1";
    self->test_tcp_2_thread.server_port = 9002;
    self->test_tcp_2_thread.data_size = 100000;
    self->test_tcp_2_thread.client_count = 10;

    test_tcp_run(&self->test_tcp_2_thread);
}

static void test_tcp_3(test_app_st* self)
{
    test_info("test tcp (unix)");

    self->test_tcp_3_thread.family = CO_NET_ADDR_FAMILY_UNIX;
    self->test_tcp_3_thread.server_address = "./test_tcp_server.sock";
    self->test_tcp_3_thread.data_size = 100000;
    self->test_tcp_3_thread.client_count = 100;

    test_tcp_run(&self->test_tcp_3_thread);
}

static void test_udp_1(test_app_st* self)
{
    test_info("test udp (ipv4)");

    self->test_udp_1_thread.family = CO_NET_ADDR_FAMILY_IPV4;
    self->test_udp_1_thread.server_address = "127.0.0.1";
    self->test_udp_1_thread.server_port = 9003;
    self->test_udp_1_thread.data_size = 100000;
    self->test_udp_1_thread.client_count = 100;

    test_udp_run(&self->test_udp_1_thread);
}

static void test_udp_2(test_app_st* self)
{
    test_info("test udp (ipv6)");

    self->test_udp_2_thread.family = CO_NET_ADDR_FAMILY_IPV6;
    self->test_udp_2_thread.server_address = "::1";
    self->test_udp_2_thread.server_port = 9004;
    self->test_udp_2_thread.data_size = 100000;
    self->test_udp_2_thread.client_count = 10;

    test_udp_run(&self->test_udp_2_thread);
}

static void test_udp_3(test_app_st* self)
{
    test_info("test udp (unix)");

    self->test_udp_3_thread.family = CO_NET_ADDR_FAMILY_UNIX;
    self->test_udp_3_thread.server_address = "./test_udp_server.sock";
    self->test_udp_3_thread.server_port = 9005;
    self->test_udp_3_thread.data_size = 100000;
    self->test_udp_3_thread.client_count = 100;

    test_udp_run(&self->test_udp_3_thread);
}

static void test_udp_4(test_app_st* self)
{
    test_info("test udp2 (ipv4)");

    self->test_udp_4_thread.family = CO_NET_ADDR_FAMILY_IPV4;
    self->test_udp_4_thread.server_address = "127.0.0.1";
    self->test_udp_4_thread.server_port = 9006;
    self->test_udp_4_thread.data_size = 100000;
    self->test_udp_4_thread.client_count = 100;

    test_udp2_run(&self->test_udp_4_thread);
}

static void test_app_init_items(test_app_st* self)
{
    size_t index = 0;

    self->item[index].thread = (co_thread_t*)&self->test_tcp_1_thread;
    self->item[index].func = test_tcp_1;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = (co_thread_t*)&self->test_tcp_2_thread;
    self->item[index].func = test_tcp_2;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = (co_thread_t*)&self->test_tcp_3_thread;
    self->item[index].func = test_tcp_3;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = (co_thread_t*)&self->test_udp_1_thread;
    self->item[index].func = test_udp_1;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = (co_thread_t*)&self->test_udp_2_thread;
    self->item[index].func = test_udp_2;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = (co_thread_t*)&self->test_udp_3_thread;
    self->item[index].func = test_udp_3;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = (co_thread_t*)&self->test_udp_4_thread;
    self->item[index].func = test_udp_4;
    self->item[index].time_limit_sec = 5 * 60;
    index++;

    self->item[index].thread = NULL;
    self->item[index].func = NULL;
    self->item[index].time_limit_sec = 0;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static void test_app_on_timer(test_app_st* self, co_timer_t* timer)
{
    (void)timer;

    test_error("**** test(%d) timeout ****", self->item_index + 1);
    exit(-1);
}

static void test_app_item_start(test_app_st* self)
{
    test_info("<<== test(%d) start ==>>", self->item_index + 1);

    if (self->timer == NULL)
    {
        self->timer = co_timer_create(
            self->item[self->item_index].time_limit_sec * 1000,
            (co_timer_fn)test_app_on_timer, false, NULL);
    }
    else
    {
        co_timer_set_time(self->timer,
            self->item[self->item_index].time_limit_sec * 1000);
    }

    self->start_time = co_get_current_time_in_msec();

    co_timer_start(self->timer);

    self->item[self->item_index].func(self);
}

static void test_app_on_item_finished(test_app_st* self, const co_event_st* event)
{
    (void)event;

    uint64_t msec = co_get_current_time_in_msec() - self->start_time;
    self->total_time += msec;

    test_info("<<== test(%d) finished: (%.4f sec) ==>>",
        self->item_index + 1, (float)msec / 1000.0f);

    co_timer_stop(self->timer);

    co_thread_t* thread = self->item[self->item_index].thread;

    if (thread != NULL)
    {
        co_thread_join(thread);
        co_net_thread_cleanup(thread);
    }

    self->item_index++;

    if (self->item[self->item_index].func == NULL)
    {
        test_info("<<== all finished - total time: (%.4f sec) ==>>",
            (float)self->total_time / 1000.0f);

        co_app_stop();

        return;
    }

    test_app_item_start(self);
}

static bool test_app_on_create(test_app_st* self)
{
    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_ID_TEST_FINISHED, (co_event_fn)test_app_on_item_finished);

    self->total_time = 0;
    self->item_index = 0;

    test_app_init_items(self);

    test_app_item_start(self);

    return true;
}

static void test_app_on_destroy(test_app_st* self)
{
    co_timer_destroy(self->timer);
    self->timer = NULL;

    int app_exit_code = 0;

    for (size_t index = 0;
        self->item[index].func != NULL;
        index++)
    {
        co_thread_t* thread = (co_thread_t*)self->item[index].thread;

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
        }
    }

    co_app_set_exit_code(app_exit_code);
}

//---------------------------------------------------------------------------//
// test_app run
//---------------------------------------------------------------------------//

int test_app_run(int argc, char** argv)
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
