#include "test_app.h"

#include <signal.h>

#ifdef CO_OS_WIN
#   ifdef CO_USE_WOLFSSL
#       pragma comment(lib, "wolfssl.lib")
#   elif defined(CO_USE_OPENSSL)
#       pragma comment(lib, "libssl.lib")
#       pragma comment(lib, "libcrypto.lib")
#   endif
#endif

void
on_signal(
    int sig
)
{
    test_info("**** signal: (%d)", sig);

    co_app_stop();
}

//---------------------------------------------------------------------------//
// main
//---------------------------------------------------------------------------//

int
main(
    int argc,
    char** argv
)
{
    signal(SIGINT, on_signal);

    return test_app_run(argc, argv);
}
