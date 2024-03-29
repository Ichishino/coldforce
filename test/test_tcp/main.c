#include "test_app.h"

#include <signal.h>

void on_signal(int sig)
{
    (void)sig;

    co_app_stop();
}

int main(int argc, char** argv)
{
    co_win_debug_crt_set_flags();
    signal(SIGINT, on_signal);

    co_log_add_category(LOG_CATEGORY_TEST_TCP_SERVER, LOG_NAME_TEST_TCP_SERVER);
    co_log_add_category(LOG_CATEGORY_TEST_TCP_CLIENT, LOG_NAME_TEST_TCP_CLIENT);
    co_log_set_level(LOG_CATEGORY_TEST_TCP_SERVER, CO_LOG_LEVEL_MAX);
//    co_log_set_level(LOG_CATEGORY_TEST_TCP_CLIENT, CO_LOG_LEVEL_MAX);

    co_core_log_set_level(CO_LOG_LEVEL_MAX);

    return test_app_run(argc, argv);
}
