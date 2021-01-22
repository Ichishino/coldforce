#ifndef CO_NET_APP_H_INCLUDED
#define CO_NET_APP_H_INCLUDED

#include <coldforce/core/co_app.h>

#include <coldforce/net/co_net.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net app
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API bool co_net_app_init(co_app_t* app,
    co_app_create_fn create_handler, co_app_destroy_fn destroy_handler);

CO_NET_API void co_net_app_cleanup(co_app_t* app);

CO_NET_API int co_net_app_start(co_app_t* app, int argc, char** argv);
CO_NET_API void co_net_app_stop(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_APP_H_INCLUDED
