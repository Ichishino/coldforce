#ifndef CO_NET_ADDR_RESOLVE_H_INCLUDED
#define CO_NET_ADDR_RESOLVE_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket_handle.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// net address resolve
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    co_address_family_t family;
    co_socket_type_t type;
    co_protocol_t protocol;
    int flags;

} co_resolve_hint_st;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_NET_API size_t co_net_addr_resolve_name(
    const char* node, uint16_t port, const co_resolve_hint_st* hint,
    co_net_addr_t* net_addr, size_t length);

CO_NET_API size_t co_net_addr_resolve_service(
    const char* node, const char* service, const co_resolve_hint_st* hint,
    co_net_addr_t* net_addr, size_t length);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#endif // CO_NET_ADDR_RESOLVE_H_INCLUDED
