#ifndef _COLDFORCE_THREAD_H_
#define _COLDFORCE_THREAD_H_

#include <coldforce/co_std.h>
#include <coldforce/co_map.h>
#include <coldforce/co_event.h>

#ifndef CO_OS_WIN
#include <pthread.h>
#endif

//---------------------------------------------------------------------------//
// Thread
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef CO_OS_WIN
typedef CO_WIN_HANDLE_T CO_THREAD_HANDLE_T;
#else
typedef pthread_t   CO_THREAD_HANDLE_T;
#endif

typedef bool(*CO_CREATE_FN)(void*, uintptr_t);
typedef void(*CO_DESTROY_FN)(void*);

typedef struct
{
    CO_EVENT_LOOP_T eloop;

    CO_THREAD_HANDLE_T handle;

    CO_CREATE_FN onCreate;
    CO_DESTROY_FN onDestroy;

    int exitCode;

} CO_THREAD_T;

typedef struct
{
    size_t objLen;

    CO_CREATE_FN onCreate;
    CO_DESTROY_FN onDestroy;

} CO_CTX_ST;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// Private
//---------------------------------------------------------------------------//

void CO_ThreadInit(CO_THREAD_T* thread, const CO_CTX_ST* ctx);
void CO_ThreadClear(CO_THREAD_T* thread);

//---------------------------------------------------------------------------//
// Public
//---------------------------------------------------------------------------//

CO_API CO_THREAD_T* CO_ThreadStart(const CO_CTX_ST* ctx, uintptr_t param);
CO_API void CO_ThreadStop(CO_THREAD_T* thread);
CO_API void CO_ThreadWait(CO_THREAD_T* thread);
CO_API void CO_ThreadDestroy(CO_THREAD_T** thread);

CO_API CO_THREAD_T* CO_ThreadGetCurrent();

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // _COLDFORCE_THREAD_H_
