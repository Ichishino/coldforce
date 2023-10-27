#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_url.h>

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
co_net_addr_remove_unix_path_file(
    const co_net_addr_t* net_addr
)
{
    if (co_net_is_unix(net_addr))
    {
        char unix_path[128];
        unix_path[0] = '\0';

        co_net_addr_get_unix_path(
            net_addr, unix_path, sizeof(unix_path));

        if (strlen(unix_path) > 0)
        {
            unlink(unix_path);
        }
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

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
    co_net_addr_family_t family
)
{
    net_addr->sa.any.ss_family = family;
}

co_net_addr_family_t
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
    if (co_net_is_ipv4(net_addr))
    {
        return (inet_pton(AF_INET,
            address, &net_addr->sa.v4.sin_addr) == 1);
    }
    else if (co_net_is_ipv6(net_addr))
    {
        return (inet_pton(AF_INET6,
            address, &net_addr->sa.v6.sin6_addr) == 1);
    }

    return false;
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
            AF_INET, &net_addr->sa.v4.sin_addr,
            buffer, (socklen_t)size) != NULL)
        {
            return true;
        }
    }
    else if (co_net_is_ipv6(net_addr))
    {
        if (inet_ntop(
            AF_INET6, &net_addr->sa.v6.sin6_addr,
            buffer, (socklen_t)size) != NULL)
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

bool
co_net_addr_set_unix_path(
    co_net_addr_t* net_addr,
    const char* path
)
{
    if (!co_net_is_unix(net_addr) ||
        strlen(path) >= sizeof(net_addr->sa.un.sun_path))
    {
        return false;
    }

    strcpy(net_addr->sa.un.sun_path, path);

    return true;
}

bool
co_net_addr_get_unix_path(
    const co_net_addr_t* net_addr,
    char* buffer,
    size_t buffer_size
)
{
    if (co_net_is_unix(net_addr))
    {
        strncpy(buffer, net_addr->sa.un.sun_path, buffer_size);

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
co_net_addr_to_string(
    const co_net_addr_t* net_addr,
    char* buffer,
    size_t size
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

        if (co_net_is_ipv6(net_addr))
        {
            snprintf(buffer, size, "[%s]:%d", address, port);
        }
        else
        {
            snprintf(buffer, size, "%s:%d", address, port);
        }
    }
    else if (co_net_is_unix(net_addr))
    {
        snprintf(buffer, size, "unix:%s", net_addr->sa.un.sun_path);
    }
    else
    {
        return false;
    }

    return true;
}

bool
co_net_addr_from_string(
    co_net_addr_family_t family,
    const char* str,
    co_net_addr_t* net_addr
)
{
    co_url_st* url = co_url_create(str);

    if (url == NULL || url->host == NULL)
    {
        co_url_destroy(url);

        return false;
    }

    co_net_addr_set_family(net_addr, family);
    co_net_addr_set_port(net_addr, url->port);

    size_t host_length = strlen(url->host);

    if (host_length > 2 &&
        url->host[0] == '[' &&
        url->host[host_length - 1] == ']')
    {
        size_t ipv6_length = host_length - 2;
        memcpy(url->host, &url->host[1], ipv6_length);
        url->host[ipv6_length] = '\0';
    }

    bool result = false;

    if (family == AF_INET)
    {
        if (inet_pton(
            family, url->host,
            &net_addr->sa.v4.sin_addr) == 1)
        {
            result = true;
        }
    }
    else if (family == AF_INET6)
    {
        if (inet_pton(
            family, url->host,
            &net_addr->sa.v6.sin6_addr) == 1)
        {
            result = true;
        }
    }
    else if (family == AF_UNIX)
    {
        if (co_net_addr_set_unix_path(net_addr, url->path))
        {
            result = true;
        }
    }

    co_url_destroy(url);

    return result;
}

bool
co_net_addr_is_equal(
    const co_net_addr_t* net_addr1,
    const co_net_addr_t* net_addr2
)
{
    if (net_addr1->sa.any.ss_family != net_addr2->sa.any.ss_family)
    {
        return false;
    }

    if (co_net_is_ipv4(net_addr1))
    {
        if ((net_addr1->sa.v4.sin_addr.s_addr ==
                net_addr2->sa.v4.sin_addr.s_addr) &&
            (net_addr1->sa.v4.sin_port ==
                net_addr2->sa.v4.sin_port))
        {
            return true;
        }
    }
    else if (co_net_is_ipv6(net_addr1))
    {
        if (memcmp(
            &net_addr1->sa.v6.sin6_addr, &net_addr2->sa.v6.sin6_addr,
            sizeof(struct in6_addr)) == 0)
        {
            if (net_addr1->sa.v6.sin6_port == net_addr2->sa.v6.sin6_port)
            {
                return true;
            }
        }
    }
    else if (co_net_is_unix(net_addr1))
    {
        if (strncmp(
            net_addr1->sa.un.sun_path, net_addr2->sa.un.sun_path,
            sizeof(net_addr1->sa.un.sun_path)) == 0)
        {
            return true;
        }
    }

    return false;
}

bool
co_net_addr_get_size(
    const co_net_addr_t* net_addr,
    size_t* size
)
{
    if (co_net_is_ipv4(net_addr))
    {
        (*size) = sizeof(struct sockaddr_in);
    }
    else if (co_net_is_ipv6(net_addr))
    {
        (*size) = sizeof(struct sockaddr_in6);
    }
    else if (co_net_is_unix(net_addr))
    {
        (*size) = sizeof(struct sockaddr_un);
    }
    else
    {
        (*size) = 0;

        return false;
    }

    return true;
}
