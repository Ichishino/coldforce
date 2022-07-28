#include "test_app.h"

#include <signal.h>

#ifdef _WIN32
#pragma comment(lib, "libssl.lib")
#pragma comment(lib, "libcrypto.lib")
#endif

void on_signal(int sig)
{
    (void)sig;

    co_app_stop();
}

int main(int argc, char** argv)
{
    co_win_crt_set_dbg();
    signal(SIGINT, on_signal);

    co_log_add_category(LOG_CATEGORY_TEST_TCP_SERVER, LOG_NAME_TEST_TCP_SERVER);
    co_log_add_category(LOG_CATEGORY_TEST_TCP_CLIENT, LOG_NAME_TEST_TCP_CLIENT);
    co_log_set_level(LOG_CATEGORY_TEST_TCP_SERVER, CO_LOG_LEVEL_MAX);
//    co_log_set_level(LOG_CATEGORY_TEST_TCP_CLIENT, CO_LOG_LEVEL_MAX);

    co_core_log_set_level(CO_LOG_LEVEL_MAX);
    co_http_log_set_level(CO_LOG_LEVEL_MAX);
    co_http2_log_set_level(CO_LOG_LEVEL_MAX);
    co_ws_log_set_level(CO_LOG_LEVEL_MAX);

    return test_app_run(argc, argv);
}
