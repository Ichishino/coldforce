#ifndef CO_NET_H_INCLUDED
#define CO_NET_H_INCLUDED

#include <coldforce/core/co.h>

#ifdef CO_OS_WIN
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   include <afunix.h>
#   include <windows.h>
#else
#   include <sys/socket.h>
#   include <netinet/in.h>
#endif

//---------------------------------------------------------------------------//
// platform
//---------------------------------------------------------------------------//

#ifdef _MSC_VER
#   ifdef CO_NET_EXPORTS
#       define CO_NET_API  __declspec(dllexport)
#   else
#       define CO_NET_API  __declspec(dllimport)
#   endif
#else
#   define CO_NET_API
#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API bool co_net_setup(void);
CO_NET_API void co_net_cleanup(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_get_local_net_addr_as_string(sock, buff) \
    co_net_addr_get_as_string(co_socket_get_local_net_addr( \
        (co_socket_t*)sock), buff) 
#define co_get_remote_net_addr_as_string(tcp_client, buff) \
    co_net_addr_get_as_string(co_tcp_get_remote_net_addr(tcp_client), buff)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#endif // CO_NET_H_INCLUDED
