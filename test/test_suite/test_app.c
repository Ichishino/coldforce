#include "test_app.h"
#include "test_tcp.h"

#include <assert.h>

//---------------------------------------------------------------------------//
// test_app
//---------------------------------------------------------------------------//

static bool test_app_on_create(test_app_t* self)
{
    test_info("<<== test tcp(ipv4) start ==>>");
    self->test_tcp_thread.family = CO_NET_ADDR_FAMILY_IPV4;
    self->test_tcp_thread.server_address = "127.0.0.1";
    self->test_tcp_thread.server_port = 9001;
    self->test_tcp_thread.data_size = 100000;
    self->test_tcp_thread.client_count = 100;
    test_tcp_run(&self->test_tcp_thread);
    co_thread_join((co_thread_t*)&self->test_tcp_thread);
    co_net_thread_cleanup((co_thread_t*)&self->test_tcp_thread);
    test_info("<<== test tcp(ipv4) complete ==>>");

    test_info("<<== test tcp(ipv6) start ==>>");
    self->test_tcp_thread.family = CO_NET_ADDR_FAMILY_IPV6;
    self->test_tcp_thread.server_address = "::1";
    self->test_tcp_thread.server_port = 9002;
    self->test_tcp_thread.data_size = 10000;
    self->test_tcp_thread.client_count = 10;
    test_tcp_run(&self->test_tcp_thread);
    co_thread_join((co_thread_t*)&self->test_tcp_thread);
    co_net_thread_cleanup((co_thread_t*)&self->test_tcp_thread);
    test_info("<<== test tcp(ipv6) complete ==>>");

    test_info("<<== test udp(ipv4) start ==>>");
    self->test_udp_thread.family = CO_NET_ADDR_FAMILY_IPV4;
    self->test_udp_thread.server_address = "127.0.0.1";
    self->test_udp_thread.server_port = 9003;
    self->test_udp_thread.data_size = 100000;
    self->test_udp_thread.client_count = 100;
    test_udp_run(&self->test_udp_thread);
    co_thread_join((co_thread_t*)&self->test_udp_thread);
    co_net_thread_cleanup((co_thread_t*)&self->test_udp_thread);
    test_info("<<== test udp(ipv4) complete ==>>");

    test_info("<<== test udp(ipv6) start ==>>");
    self->test_udp_thread.family = CO_NET_ADDR_FAMILY_IPV6;
    self->test_udp_thread.server_address = "::1";
    self->test_udp_thread.server_port = 9004;
    self->test_udp_thread.data_size = 10000;
    self->test_udp_thread.client_count = 10;
    test_udp_run(&self->test_udp_thread);
    co_thread_join((co_thread_t*)&self->test_udp_thread);
    co_net_thread_cleanup((co_thread_t*)&self->test_udp_thread);
    test_info("<<== test udp(ipv6) complete ==>>");

    return false;
}

static void test_app_on_destroy(test_app_t* self)
{
    co_thread_stop((co_thread_t*)&self->test_tcp_thread);
    co_thread_stop((co_thread_t*)&self->test_udp_thread);

    co_thread_join((co_thread_t*)&self->test_tcp_thread);
    co_thread_join((co_thread_t*)&self->test_udp_thread);

    co_net_thread_cleanup((co_thread_t*)&self->test_tcp_thread);
    co_net_thread_cleanup((co_thread_t*)&self->test_udp_thread);
}

//---------------------------------------------------------------------------//
// test_app run
//---------------------------------------------------------------------------//

int test_app_run(int argc, char** argv)
{
    co_tls_debug_mem_check();

    co_core_log_set_level(CO_LOG_LEVEL_MAX);
//    co_tcp_log_set_level(CO_LOG_LEVEL_INFO);
//    co_udp_log_set_level(CO_LOG_LEVEL_INFO);
//    co_tls_log_set_level(CO_LOG_LEVEL_MAX);

    co_log_add_category(TEST_LOG_CATEGORY, "TEST");
    co_log_set_level(TEST_LOG_CATEGORY, CO_LOG_LEVEL_MAX);

    co_tls_setup();

    test_app_t test_app = { 0 };

    int exit_code =
        co_net_app_start(
            (co_app_t*)&test_app, "test_app",
            (co_app_create_fn)test_app_on_create,
            (co_app_destroy_fn)test_app_on_destroy,
            argc, argv);

    co_tls_cleanup();

    return exit_code;
}
