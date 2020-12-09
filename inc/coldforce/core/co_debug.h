#ifndef CO_DEBUG_H_INCLUDED
#define CO_DEBUG_H_INCLUDED

//---------------------------------------------------------------------------//
// debug tools
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef _WIN32
#   ifdef _DEBUG
#       define _CRTDBG_MAP_ALLOC
#       include <crtdbg.h>
#       define co_win_crt_set_dbg() _CrtSetDbgFlag( \
    _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF)
#   else
#       define co_win_crt_set_dbg() ((void)0)
#   endif
#else
#   define co_win_crt_set_dbg() ((void)0)
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#endif // CO_DEBUG_H_INCLUDED
