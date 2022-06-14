#include <coldforce/core/co_std.h>

#include <coldforce/http2/co_http2_log.h>

//---------------------------------------------------------------------------//
// http2 log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_http2_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_HTTP2_LOG_CATEGORY, level);

    co_log_add_category(
        CO_HTTP2_LOG_CATEGORY,
        CO_HTTP2_LOG_CATEGORY_NAME);
}
