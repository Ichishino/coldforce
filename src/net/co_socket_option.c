#include <coldforce/core/co_std.h>

#include <coldforce/net/co_socket_option.h>

#ifndef CO_OS_WIN
#include <netinet/tcp.h>
#endif

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
    size_t data_size
)
{
    return co_socket_handle_set_option(
        sock->handle, level, name, data, data_size);
}

bool
co_socket_option_get(
    const co_socket_t* sock,
    int level,
    int name,
    void* buffer,
    size_t* buffer_size
)
{
    return co_socket_handle_get_option(
        sock->handle, level, name, buffer, buffer_size);
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
    size_t value_size = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_REUSEADDR, &value, &value_size))
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
    size_t value_size = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_KEEPALIVE, &value, &value_size))
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
co_socket_option_set_send_buffer(
    co_socket_t* sock,
    size_t buffer_size
)
{
    int value = (int)buffer_size;

    return co_socket_option_set(
        sock, SOL_SOCKET, SO_SNDBUF, &value, sizeof(value));
}

bool
co_socket_option_get_send_buffer(
    const co_socket_t* sock,
    size_t* buffer_size
)
{
    int value = 0;
    size_t value_size = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_SNDBUF, &value, &value_size))
    {
        return false;
    }

    *buffer_size = (size_t)value;

    return true;
}

//---------------------------------------------------------------------------//
// SO_RCVBUF
//---------------------------------------------------------------------------//

bool
co_socket_option_set_receive_buffer(
    co_socket_t* sock,
    size_t buffer_size
)
{
    int value = (int)buffer_size;

    return co_socket_option_set(
        sock, SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
}

bool
co_socket_option_get_receive_buffer(
    const co_socket_t* sock,
    size_t* buffer_size
)
{
    int value = 0;
    size_t value_size = sizeof(value);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_RCVBUF, &value, &value_size))
    {
        return false;
    }

    *buffer_size = (size_t)value;

    return true;
}

//---------------------------------------------------------------------------//
// SO_ERROR
//---------------------------------------------------------------------------//

bool
co_socket_option_get_error(
    const co_socket_t* sock,
    int* error_code
)
{
    size_t value_size = sizeof(int);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_ERROR, error_code, &value_size))
    {
        return false;
    }

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
    size_t value_size = sizeof(struct linger);

    if (!co_socket_option_get(
        sock, SOL_SOCKET, SO_LINGER, linger, &value_size))
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
    size_t value_size = sizeof(value);

    if (!co_socket_option_get(
        sock, IPPROTO_TCP, TCP_NODELAY, &value, &value_size))
    {
        return false;
    }

    *enable = (value == 1) ? true : false;

    return true;
}
