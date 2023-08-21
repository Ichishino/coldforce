#ifndef CO_URL_H_INCLUDED
#define CO_URL_H_INCLUDED

#include <coldforce/core/co_string_map.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// url
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

typedef struct
{
    char* scheme;
    char* user;
    char* password;
    char* host;
    uint16_t port;
    char* path;
    char* query;
    char* fragment;

    char* src;
    char* origin;
    char* host_and_port;
    char* path_and_query;

} co_url_st;

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
co_url_st*
co_url_create(
    const char* str
);

CO_NET_API
void
co_url_destroy(
    co_url_st* url
);

CO_NET_API
bool
co_url_component_encode(
    const char* src,
    size_t src_length,
    char** dest,
    size_t* dest_length
);

CO_NET_API
bool
co_url_component_decode(
    const char* src,
    size_t src_length,
    char** dest,
    size_t* dest_length
);

CO_NET_API
co_string_map_t*
co_url_query_parse(
    const char* src,
    bool unescape
);

CO_NET_API
char*
co_url_query_to_string(
    const co_string_map_t* query_map,
    bool escape
);

CO_NET_API
bool
co_url_to_net_addr(
    const co_url_st* url,
    int address_family,
    co_net_addr_t* net_addr
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_URL_H_INCLUDED
