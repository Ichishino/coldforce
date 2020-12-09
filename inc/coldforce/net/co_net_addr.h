#ifndef CO_NET_ADDR_H_INCLUDED
#define CO_NET_ADDR_H_INCLUDED

#include <coldforce/net/co_net.h>

#ifdef CO_OS_WIN
#include <afunix.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#endif

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net address
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef enum
{
    CO_ADDRESS_FAMILY_UNSPEC = AF_UNSPEC,
    CO_ADDRESS_FAMILY_IPV4 = AF_INET,
    CO_ADDRESS_FAMILY_IPV6 = AF_INET6,
    CO_ADDRESS_FAMILY_UNIX = AF_UNIX

} co_address_family_t;

typedef struct
{
    union
    {
        struct sockaddr_in v4;
        struct sockaddr_in6 v6;
        struct sockaddr_un un;
        struct sockaddr_storage any;

    } sa;

} co_net_addr_t;

#define CO_NET_ADDR_INIT  { 0 }

#define CO_NET_ADDR_INIT_IPV4 \
    { \
        .sa.v4.sin_family = CO_ADDRESS_FAMILY_IPV4, \
        .sa.v4.sin_addr = { 0 }, \
        .sa.v4.sin_port = 0 \
    }

#define CO_NET_ADDR_INIT_IPV6 \
    { \
        .sa.v6.sin6_family = CO_ADDRESS_FAMILY_IPV6, \
        .sa.v6.sin6_addr = { 0 }, \
        .sa.v6.sin6_port = 0 \
        .sa.v6.sin6_flowinfo = 0, \
        .sa.v6.sin6_scope_id = 0 \
    }

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API void co_net_addr_init(co_net_addr_t* net_addr);

CO_NET_API void co_net_addr_set_family(co_net_addr_t* net_addr, co_address_family_t family);
CO_NET_API co_address_family_t co_net_addr_get_family(const co_net_addr_t* net_addr);

CO_NET_API bool co_net_addr_set_address(co_net_addr_t* new_addr, const char* address);
CO_NET_API bool co_net_addr_get_address(const co_net_addr_t* net_addr, char* buffer, size_t size);

CO_NET_API bool co_net_addr_set_port(co_net_addr_t* net_addr, uint16_t port);
CO_NET_API bool co_net_addr_get_port(const co_net_addr_t* net_addr, uint16_t* port);

CO_NET_API void co_net_addr_set_unix_path(co_net_addr_t* net_addr, const char* path);
CO_NET_API bool co_net_addr_get_unix_path(const co_net_addr_t* net_addr, char* path);

CO_NET_API bool co_net_addr_set_scope_id(co_net_addr_t* net_addr, uint32_t scope_id);
CO_NET_API bool co_net_addr_get_scope_id(const co_net_addr_t* net_addr, uint32_t* scope_id);

CO_NET_API bool co_net_addr_get_as_string(const co_net_addr_t* net_addr, char* buffer);

CO_NET_API bool co_net_addr_is_equal(
    const co_net_addr_t* net_addr1, const co_net_addr_t* net_addr2);

#define co_get_local_net_addr_as_string(sock, buff) \
    co_net_addr_get_as_string(co_socket_get_local_net_addr( \
        (co_socket_t*)sock), buff)
#define co_get_remote_net_addr_as_string(tcp_client, buff) \
    co_net_addr_get_as_string(co_tcp_get_remote_net_addr(tcp_client), buff)

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_NET_ADDR_H_INCLUDED
