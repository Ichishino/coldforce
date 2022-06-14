#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_ws_log.h>

//---------------------------------------------------------------------------//
// websocket log
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_ws_log_set_level(
    int level
)
{
    co_log_set_level(
        CO_WS_LOG_CATEGORY, level);

    co_log_add_category(
        CO_WS_LOG_CATEGORY,
        CO_WS_LOG_CATEGORY_NAME);
}
