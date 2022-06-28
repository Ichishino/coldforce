#ifndef CO_CONFIG_H_INCLUDED
#define CO_CONFIG_H_INCLUDED

#include <coldforce/core/co.h>
#include <coldforce/core/co_ss_map.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// config
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_CONFIG_KEY_MAX_LENGTH        256 
#define CO_CONFIG_VALUE_MAX_LENGTH      1024
#define CO_CONFIG_LINE_MAX_LENGTH   \
    (CO_CONFIG_KEY_MAX_LENGTH + CO_CONFIG_VALUE_MAX_LENGTH + 1024)

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_API
co_ss_map_t*
co_config_read_file(
    const char* file_path
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_CONFIG_H_INCLUDED
