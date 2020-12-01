#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_addr.h>

#ifndef CO_OS_WIN
#include <arpa/inet.h>
#endif

//---------------------------------------------------------------------------//
// net address
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define co_net_is_ipv4(net_addr)    (net_addr->sa.any.ss_family == AF_INET)
#define co_net_is_ipv6(net_addr)    (net_addr->sa.any.ss_family == AF_INET6)
#define co_net_is_unix(net_addr)    (net_addr->sa.any.ss_family == AF_UNIX)

void
co_net_addr_init(
    co_net_addr_t* net_addr
)
{
    memset(net_addr, 0x00, sizeof(co_net_addr_t));
}

void
co_net_addr_set_family(
    co_net_addr_t* net_addr,
    co_address_family_t family
)
{
    net_addr->sa.any.ss_family = family;

    if (co_net_is_ipv4(net_addr))
    {
        net_addr->sa.v4.sin_addr.s_addr = INADDR_ANY;
        net_addr->sa.v4.sin_port = htons(0);
    }
    else if (co_net_is_ipv6(net_addr))
    {
        net_addr->sa.v6.sin6_addr = in6addr_any;
        net_addr->sa.v6.sin6_port = htons(0);
        net_addr->sa.v6.sin6_flowinfo = 0;
        net_addr->sa.v6.sin6_scope_id = 0;
    }
}

co_address_family_t
co_net_addr_get_family(
    const co_net_addr_t* net_addr
)
{
    return net_addr->sa.any.ss_family;
}

bool
co_net_addr_set_address(
    co_net_addr_t* net_addr,
    const char* address
)
{
    if (inet_pton(AF_INET, address, &net_addr->sa.v4.sin_addr) == 1)
    {
        net_addr->sa.v4.sin_family = AF_INET;
    }
    else if (inet_pton(AF_INET6, address, &net_addr->sa.v6.sin6_addr) == 1)
    {
        net_addr->sa.v6.sin6_family = AF_INET6;
    }
    else
    {
        return false;
    }

    return true;
}

bool
co_net_addr_get_address(
    const co_net_addr_t* net_addr,
    char* buffer,
    size_t size
)
{
    if (co_net_is_ipv4(net_addr))
    {
        if (inet_ntop(
            AF_INET, &net_addr->sa.v4.sin_addr, buffer, size) != NULL)
        {
            return true;
        }
    }
    else if (co_net_is_ipv6(net_addr))
    {
        if (inet_ntop(
            AF_INET6, &net_addr->sa.v6.sin6_addr, buffer, size) != NULL)
        {
            return true;
        }
    }

    return false;
}

bool
co_net_addr_set_port(
    co_net_addr_t* net_addr,
    uint16_t port
)
{
    if (co_net_is_ipv4(net_addr))
    {
        net_addr->sa.v4.sin_port = htons(port);
    }
    else if (co_net_is_ipv6(net_addr))
    {
        net_addr->sa.v6.sin6_port = htons(port);
    }
    else
    {
        return false;
    }

    return true;
}

bool
co_net_addr_get_port(
    const co_net_addr_t* net_addr,
    uint16_t* port
)
{
    if (co_net_is_ipv4(net_addr))
    {
        *port = ntohs(net_addr->sa.v4.sin_port);
    }
    else if (co_net_is_ipv6(net_addr))
    {
        *port = ntohs(net_addr->sa.v6.sin6_port);
    }
    else
    {
        return false;
    }

    return true;
}

void
co_net_addr_set_unix_path(
    co_net_addr_t* net_addr,
    const char* path
)
{
    net_addr->sa.any.ss_family = AF_UNIX;

    strcpy(net_addr->sa.un.sun_path, path);
}

bool
co_net_addr_get_unix_path(
    const co_net_addr_t* net_addr,
    char* path
)
{
    if (co_net_is_unix(net_addr))
    {
        strcpy(path, net_addr->sa.un.sun_path);

        return true;
    }
    else
    {
        return false;
    }
}

bool
co_net_addr_set_scope_id(
    co_net_addr_t* net_addr,
    uint32_t scope_id
)
{
    if (co_net_is_ipv6(net_addr))
    {
        net_addr->sa.v6.sin6_scope_id = scope_id;

        return true;
    }
    else
    {
        return false;
    }
}

bool
co_net_addr_get_scope_id(
    const co_net_addr_t* net_addr,
    uint32_t* scope_id
)
{
    if (co_net_is_ipv6(net_addr))
    {
        *scope_id = (uint32_t)net_addr->sa.v6.sin6_scope_id;

        return true;
    }
    else
    {
        return false;
    }
}

bool
co_net_addr_get_as_string(
    const co_net_addr_t* net_addr,
    char* buffer
)
{
    if (co_net_is_ipv4(net_addr) || co_net_is_ipv6(net_addr))
    {
        char address[256];

        if (!co_net_addr_get_address(net_addr, address, sizeof(address)))
        {
            return false;
        }

        uint16_t port;

        if (!co_net_addr_get_port(net_addr, &port))
        {
            return false;
        }

        sprintf(buffer, "%s%c%d",
            address, (co_net_is_ipv4(net_addr) ? ':' : '.'), port);
    }
    else if (co_net_is_unix(net_addr))
    {
        sprintf(buffer, "unix:%s", net_addr->sa.un.sun_path);
    }
    else
    {
        return false;
    }

    return true;
}
