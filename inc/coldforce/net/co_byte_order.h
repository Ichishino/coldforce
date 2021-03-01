#ifndef CO_BYTE_ORDER_H_INCLUDED
#define CO_BYTE_ORDER_H_INCLUDED

#include <coldforce/net/co_net.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef CO_LITTLE_ENDIAN

#define co_byte_order_16_swap(n) \
    (((n & 0x00ff) << 8) | ((n & 0xff00) >> 8))
#define co_byte_order_32_swap(n) \
    (((n & 0x000000ff) << 24) | ((n & 0x0000ff00) << 8) | \
     ((n & 0x00ff0000) >> 8)  | ((n & 0xff000000) >>24))
#define co_byte_order_64_swap(n) \
    (((n & 0x00000000000000ff) << 56) | ((n & 0x000000000000ff00) << 40) | \
     ((n & 0x0000000000ff0000) << 24) | ((n & 0x00000000ff000000) << 8) | \
     ((n & 0x000000ff00000000) >> 8)  | ((n & 0x0000ff0000000000) >> 24) | \
     ((n & 0x00ff000000000000) >> 40) | ((n & 0xff00000000000000) >> 56))

#define co_byte_order_16_network_to_host(n) co_byte_order_16_swap(n)
#define co_byte_order_16_host_to_network(n) co_byte_order_16_swap(n)
#define co_byte_order_32_network_to_host(n) co_byte_order_32_swap(n)
#define co_byte_order_32_host_to_network(n) co_byte_order_32_swap(n)
#define co_byte_order_64_network_to_host(n) co_byte_order_64_swap(n)
#define co_byte_order_64_host_to_network(n) co_byte_order_64_swap(n)

#else

#define co_byte_order_16_network_to_host(n) (n)
#define co_byte_order_16_host_to_network(n) (n)
#define co_byte_order_32_network_to_host(n) (n)
#define co_byte_order_32_host_to_network(n) (n)
#define co_byte_order_64_network_to_host(n) (n)
#define co_byte_order_64_host_to_network(n) (n)

#endif

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_BYTE_ORDER_H_INCLUDED
