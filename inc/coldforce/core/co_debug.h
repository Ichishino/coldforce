#ifndef CO_DEBUG_H_INCLUDED
#define CO_DEBUG_H_INCLUDED

//---------------------------------------------------------------------------//
// debug tools
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

#if defined(_WIN32) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define co_win_debug_crt_set_flags() \
    _CrtSetDbgFlag( \
        _CRTDBG_ALLOC_MEM_DF | \
        _CRTDBG_CHECK_ALWAYS_DF | \
        _CRTDBG_LEAK_CHECK_DF)
#else
#define co_win_debug_crt_set_flags() ((void)0)
#endif // _WIN32 && _DEBUG

#ifdef _DEBUG

#include <stdlib.h>

inline void*
co_mem_alloc_dbg(
    size_t size,
    const char* file,
    int line
)
{
#ifdef _WIN32
    return _malloc_dbg(size, _NORMAL_BLOCK, file, line);
#else
    (void)file;
    (void)line;
    return malloc(size);
#endif // _WIN32
}

inline void*
co_mem_realloc_dbg(
    void* ptr,
    size_t size,
    const char* file,
    int line
)
{
#ifdef _WIN32
    return _realloc_dbg(ptr, size, _NORMAL_BLOCK, file, line);
#else
    (void)file;
    (void)line;
    return realloc(ptr, size);
#endif // _WIN32
}

inline void
co_mem_free_dbg(
    void* ptr,
    const char* file,
    int line
)
{
    (void)file;
    (void)line;

#ifdef _WIN32
    _free_dbg(ptr, _NORMAL_BLOCK);
#else
    free(ptr);
#endif // _WIN32
}

#endif // CO_DEBUG

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#endif // CO_DEBUG_H_INCLUDED
