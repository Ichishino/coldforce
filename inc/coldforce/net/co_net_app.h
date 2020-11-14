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

CO_NET_API co_app_t* co_net_app_create(co_ctx_st* ctx);

CO_NET_API void co_net_app_destroy(co_app_t* app);

CO_NET_API int co_net_app_start(co_ctx_st* ctx, co_app_param_st* param);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_APP_H_INCLUDED
