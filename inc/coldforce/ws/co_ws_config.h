#ifndef CO_WS_CONFIG_H_INCLUDED
#define CO_WS_CONFIG_H_INCLUDED

#include <coldforce/ws/co_ws.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// websocket config
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WS_CONFIG_MAX_PAYLOAD_SIZE       (32 * 1024 * 1024)

typedef struct
{
    uint64_t max_payload_size;

} co_ws_config_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_WS_API void co_ws_config_set_max_payload_size(uint64_t max_payload_size);
CO_WS_API uint64_t co_ws_config_get_max_payload_size(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_WS_CONFIG_H_INCLUDED
