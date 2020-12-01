#include <coldforce/core/co_std.h>

#include <coldforce/net/co_socket_handle.h>
#include <coldforce/net/co_net_worker.h>

#ifndef CO_OS_WIN
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#endif

//---------------------------------------------------------------------------//
// socket handle
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

int
co_socket_get_error(
    void
)
{
#ifdef CO_OS_WIN
    return WSAGetLastError();
#else
    return errno;
#endif
}

co_socket_handle_t
co_socket_handle_create(
    int family,
    int type,
    int protocol
)
{
    co_socket_handle_t handle = socket(family, type, protocol);

    if (handle != CO_SOCKET_INVALID_HANDLE)
    {
        CO_DEBUG_SOCKET_COUNTER_INC();

#ifdef CO_OS_MAC
        co_socket_option_set_sigpipe(handle, false);
#endif
    }

    return handle;
}

void
co_socket_handle_close(
    co_socket_handle_t handle
)
{
    if (handle != CO_SOCKET_INVALID_HANDLE)
    {
        CO_DEBUG_SOCKET_COUNTER_DEC();

#ifdef CO_OS_WIN
        closesocket(handle);
#else
        close(handle);
#endif
    }
}

bool
co_socket_handle_shutdown(
    co_socket_handle_t handle,
    int how
)
{
    int result = shutdown(handle, how);

    return (result == 0);
}

bool
co_socket_handle_bind(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr
)
{
    int result = bind(
        handle, (const struct sockaddr*)net_addr, sizeof(co_net_addr_t));

    return (result == 0);
}

bool
co_socket_handle_listen(
    co_socket_handle_t handle,
    int backlog
)
{
    int result = listen(handle, backlog);

    return (result == 0);
}

co_socket_handle_t
co_socket_handle_accept(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
)
{
    socklen_t net_addr_size = sizeof(co_net_addr_t);

    co_socket_handle_t remote_handle = accept(
        handle, (struct sockaddr*)net_addr, &net_addr_size);

    if (remote_handle != CO_SOCKET_INVALID_HANDLE)
    {
        CO_DEBUG_SOCKET_COUNTER_INC();

#ifdef CO_OS_MAC
        co_socket_option_set_sigpipe(new_sock, false);
#endif
    }

    return remote_handle;
}

bool
co_socket_handle_connect(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr
)
{
    int result = connect(handle,
        (const struct sockaddr*)net_addr, sizeof(co_net_addr_t));

    return (result == 0);
}

ssize_t
co_socket_handle_send(
    co_socket_handle_t handle,
    const void* data,
    size_t data_size,
    int flags
)
{
#ifdef CO_OS_LINUX
    flags |= MSG_NOSIGNAL;
#endif
    ssize_t result = send(handle, data, (int)data_size, flags);

    return result;
}

ssize_t
co_socket_handle_receive(
    co_socket_handle_t handle,
    void* buffer,
    size_t buffer_size,
    int flags
)
{
    ssize_t result = recv(handle, buffer, (int)buffer_size, flags);

    return result;
}

ssize_t
co_socket_handle_send_to(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr,
    const void* data,
    size_t data_size,
    int flags
)
{
    ssize_t result = sendto(handle, data, (int)data_size, flags,
        (const struct sockaddr*)net_addr, sizeof(co_net_addr_t));

    return result;
}

ssize_t
co_socket_handle_receive_from(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr,
    void* buffer,
    size_t buffer_size,
    int flags
)
{
    socklen_t net_addr_size = sizeof(co_net_addr_t);

    ssize_t result = recvfrom(handle, buffer, (int)buffer_size, flags,
        (struct sockaddr*)net_addr, &net_addr_size);

    return result;
}

bool
co_socket_handle_set_option(
    co_socket_handle_t handle,
    int level,
    int name,
    const void* data,
    size_t data_size)
{
    int result = setsockopt(
        handle, level, name, data, (socklen_t)data_size);

    return (result == 0);
}

bool
co_socket_handle_get_option(
    co_socket_handle_t handle,
    int level,
    int name,
    void* buffer,
    size_t* buffer_size
)
{
    int result = getsockopt(
        handle, level, name, buffer, (socklen_t*)buffer_size);

    return (result == 0);
}

bool
co_socket_handle_get_local_net_addr(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
)
{
    socklen_t net_addr_size = sizeof(co_net_addr_t);

    int result = getsockname(
        handle, (struct sockaddr*)net_addr, &net_addr_size);

    return (result == 0);
}

bool
co_socket_handle_get_remote_net_addr(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
)
{
    socklen_t net_addr_size = sizeof(co_net_addr_t);

    int result = getpeername(
        handle, (struct sockaddr*)net_addr, &net_addr_size);

    return (result == 0);
}

bool
co_socket_handle_set_blocking(
    co_socket_handle_t handle,
    bool enable
)
{
#ifdef CO_OS_WIN

    u_long value = (enable ? 0 : 1);

    return (ioctlsocket(handle, FIONBIO, &value) == 0);

#else

    int flag = fcntl(handle, F_GETFL, 0);

    if (enable)
    {
        flag &= ~O_NONBLOCK;
    }
    else
    {
        flag |= O_NONBLOCK;
    }

    return (fcntl(handle, F_SETFL, flag) != -1);

#endif
}
