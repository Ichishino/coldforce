#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_ws_config.h>

//---------------------------------------------------------------------------//
// websocket config
//---------------------------------------------------------------------------//

static co_ws_config_t ws_config =
{
    CO_WS_CONFIG_MAX_PAYLOAD_SIZE
};

void
co_ws_config_set_max_payload_size(
    uint64_t max_payload_size
)
{
    ws_config.max_payload_size = max_payload_size;
}

uint64_t
co_ws_config_get_max_payload_size(
    void
)
{
    return ws_config.max_payload_size;
}
