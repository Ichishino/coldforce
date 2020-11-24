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

CO_NET_API void co_net_thread_init(co_thread_t* thread,
    co_create_fn create_handler, co_destroy_fn destroy_handler);

CO_NET_API void co_net_thread_cleanup(co_thread_t* thread);

CO_NET_API bool co_net_thread_start(co_thread_t* thread, uintptr_t param);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_THREAD_H_INCLUDED
