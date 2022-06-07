#include <coldforce/core/co_std.h>

#include <coldforce/tls/co_tls_log.h>

//---------------------------------------------------------------------------//
// tls log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_tls_log_enable(
    bool enable
)
{
    co_log_add_category(
        CO_TLS_LOG_CATEGORY,
        CO_TLS_LOG_CATEGORY_NAME);

    co_log_set_enable(
        CO_TLS_LOG_CATEGORY, enable);
}
