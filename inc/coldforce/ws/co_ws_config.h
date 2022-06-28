#ifndef CO_WS_CONFIG_H_INCLUDED
#define CO_WS_CONFIG_H_INCLUDED

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket config
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WS_CONFIG_DEFAULT_MAX_RECEIVE_PAYLOAD_SIZE    (32 * 1024 * 1024)

typedef struct
{
    size_t max_receive_payload_size;

} co_ws_config_t;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_WS_API
void
co_ws_config_set_max_receive_payload_size(
    size_t max_receive_payload_size
);

CO_WS_API
size_t
co_ws_config_get_max_receive_payload_size(
    void
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_CONFIG_H_INCLUDED
