#include <coldforce/core/co_std.h>

#include <coldforce/net/co_socket_option.h>

//---------------------------------------------------------------------------//
// socket option
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// any option
//---------------------------------------------------------------------------//

bool
co_socket_option_set(
    co_socket_t* sock,
    int level,
    int name,
    const void* data,
    size_t length
)
{
    return co_socket_handle_set_option(
        sock->handle, level, name, data, length);
}

bool
co_socket_option_get(
    const co_socket_t* sock,
    int level,
    int name,
    void* buffer,
    size_t* length
)
{
    return co_socket_handle_get_option(
        sock->handle, level, name, buffer, length);
}

//---------------------------------------------------------------------------//
// SO_REUSEADDR
//---------------------------------------------------------------------------//

bool
co_socket_option_set_reuse_addr(
    co_socket_t* sock,
    bool enable
)
{
    int value = enable ? 1 : 0;

    return co_socket_option_set(
        sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
}

bool
co_socket_option_get_reuse_addr(
    const co_socket_t* sock,
    bool* enable
)
{
    int value = 0;
    size_t length = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_REUSEADDR, &value, &length))
    {
        return false;
    }

    *enable = (value == 1) ? true : false;

    return true;
}

//---------------------------------------------------------------------------//
// SO_KEEPALIVE
//---------------------------------------------------------------------------//

bool
co_socket_option_set_keep_alive(
    co_socket_t* sock,
    bool enable
)
{
    int value = enable ? 1 : 0;

    return co_socket_option_set(
        sock, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value));
}

bool
co_socket_option_get_keep_alive(
    const co_socket_t* sock,
    bool* enable
)
{
    int value = 0;
    size_t length = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_KEEPALIVE, &value, &length))
    {
        return false;
    }

    *enable = (value == 1) ? true : false;

    return true;
}

//---------------------------------------------------------------------------//
// SO_SNDBUF
//---------------------------------------------------------------------------//

bool
co_socket_option_set_send_buff_size(
    co_socket_t* sock,
    size_t size
)
{
    int value = (int)size;

    return co_socket_option_set(
        sock, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value));
}

bool
co_socket_option_get_send_buff_size(
    const co_socket_t* sock,
    size_t* size
)
{
    int value = 0;
    size_t length = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_SNDBUF, &value, &length))
    {
        return false;
    }

    *size = (size_t)value;

    return true;
}

//---------------------------------------------------------------------------//
// SO_RCVBUF
//---------------------------------------------------------------------------//

bool
co_socket_option_set_receive_buff_size(
    co_socket_t* sock,
    size_t size
)
{
    int value = (int)size;

    return co_socket_option_set(
        sock, SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
}

bool
co_socket_option_get_receive_buff_size(
    const co_socket_t* sock,
    size_t* size
)
{
    int value = 0;
    size_t length = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_RCVBUF, &value, &length))
    {
        return false;
    }

    *size = (size_t)value;

    return true;
}

//---------------------------------------------------------------------------//
// SO_LINGER
//---------------------------------------------------------------------------//

bool
co_socket_option_set_linger(
    co_socket_t* sock,
    const struct linger* linger
)
{
    return co_socket_option_set(
        sock, SOL_SOCKET, SO_LINGER, linger, sizeof(struct linger));
}

bool
co_socket_option_get_linger(
    const co_socket_t* sock,
    struct linger* linger
)
{
    size_t length = sizeof(struct linger);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_LINGER, linger, &length))
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------//
// TCP_NODELAY
//---------------------------------------------------------------------------//

bool
co_socket_option_set_tcp_no_delay(
    co_socket_t* sock,
    bool enable
)
{
    int value = enable ? 1 : 0;

    return co_socket_option_set(
        sock, IPPROTO_TCP, TCP_NODELAY, &value, sizeof(value));
}

bool
co_socket_option_get_tcp_no_delay(
    const co_socket_t* sock,
    bool* enable
)
{
    int value = 0;
    size_t length = sizeof(value);

    if (!co_socket_option_get(
        sock, IPPROTO_TCP, TCP_NODELAY, &value, &length))
    {
        return false;
    }

    *enable = (value == 1) ? true : false;

    return true;
}
