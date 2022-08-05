#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_addr_resolve.h>

#ifndef CO_OS_WIN
#   include <unistd.h>
#   include <netdb.h>
#   include <netinet/tcp.h>
#   include <arpa/inet.h>
#endif

//---------------------------------------------------------------------------//
// net address resolve
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

size_t
co_net_addr_resolve(
    const char* node,
    const char* service,
    const co_resolve_hint_st* hint,
    co_net_addr_t* net_addr,
    size_t count
)
{
    struct addrinfo in = { 0 };

    if (hint != NULL)
    {
        in.ai_flags = hint->flags;
        in.ai_family = hint->family;
        in.ai_socktype = hint->type;
        in.ai_protocol = hint->protocol;
    }

    if (node == NULL)
    {
        in.ai_flags |= AI_PASSIVE;
    }

    struct addrinfo* out = NULL;

    if (getaddrinfo(node, service, &in, &out) != 0)
    {
        return 0;
    }

    size_t data_count = 0;

    for (struct addrinfo* ai = out;
        (ai != NULL) && (data_count < count);
        ai = ai->ai_next)
    {
        memcpy(&net_addr[data_count], ai->ai_addr, ai->ai_addrlen);
        data_count++;
    }

    freeaddrinfo(out);

    return data_count;
}
