#include <coldforce/core/co_std.h>

#include <coldforce/ws/co_ws_config.h>

//---------------------------------------------------------------------------//
// websocket config
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

static co_ws_config_t ws_config =
{
    CO_WS_CONFIG_DEFAULT_MAX_RECEIVE_PAYLOAD_SIZE
};

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

void
co_ws_config_set_max_receive_payload_size(
    size_t max_receive_payload_size
)
{
    ws_config.max_receive_payload_size = max_receive_payload_size;
}

size_t
co_ws_config_get_max_receive_payload_size(
    void
)
{
    return ws_config.max_receive_payload_size;
}
