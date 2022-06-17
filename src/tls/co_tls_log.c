#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_log.h>

//---------------------------------------------------------------------------//
// tls log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_tls_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_LOG_CATEGORY_TLS, level);

    co_log_add_category(
        CO_LOG_CATEGORY_TLS,
        CO_LOG_CATEGORY_NAME_TLS);
}

