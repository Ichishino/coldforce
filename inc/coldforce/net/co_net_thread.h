#ifndef CO_NET_THREAD_H_INCLUDED
#define CO_NET_THREAD_H_INCLUDED

#include <coldforce/core/co_thread.h>

#include <coldforce/net/co_net.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API co_thread_t* co_net_thread_start(co_ctx_st* ctx, uintptr_t param);
CO_NET_API void co_net_thread_destroy(co_thread_t* thread);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_THREAD_H_INCLUDED
