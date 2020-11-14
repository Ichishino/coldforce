#include <coldforce/core/co_std.h>

#include <coldforce/net/co_socket_handle.h>

#ifdef CO_DEBUG
#include <coldforce/net/co_net_worker.h>
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
    co_address_family_t family,
    co_socket_type_t type,
    co_protocol_t protocol
)
{
#ifdef CO_DEBUG
    co_thread_t* thread = co_thread_get_current();
    ((co_net_worker_t*)thread->event_worker)->sock_counter++;
#endif

    co_socket_handle_t handle = socket(family, type, protocol);

#ifdef CO_OS_MAC
    if (handle != CO_SOCKET_HANDLE_INVALID)
    {
        co_socket_option_set_sigpipe(handle, false);
    }
#endif

    return handle;
}

void
co_socket_handle_close(
    co_socket_handle_t handle
)
{
    if (handle != CO_SOCKET_INVALID_HANDLE)
    {
#ifdef CO_DEBUG
        co_thread_t* thread = co_thread_get_current();
        ((co_net_worker_t*)thread->event_worker)->sock_counter--;
#endif

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
    co_socket_shutdown_how_t how
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
    socklen_t net_addr_length = sizeof(co_net_addr_t);

    co_socket_handle_t remote_handle = accept(
        handle, (struct sockaddr*)net_addr, &net_addr_length);

    if (remote_handle != CO_SOCKET_INVALID_HANDLE)
    {
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
    size_t length,
    int flags
)
{
    ssize_t result = send(handle, data, (int)length, flags);

    return result;
}

ssize_t
co_socket_handle_receive(
    co_socket_handle_t handle,
    void* buffer,
    size_t length,
    int flags
)
{
    ssize_t result = recv(handle, buffer, (int)length, flags);

    return result;
}

ssize_t
co_socket_handle_send_to(
    co_socket_handle_t handle,
    const co_net_addr_t* net_addr,
    const void* data,
    size_t length,
    int flags
)
{
    ssize_t result = sendto(handle, data, (int)length, flags,
        (const struct sockaddr*)net_addr, sizeof(co_net_addr_t));

    return result;
}

ssize_t
co_socket_handle_receive_from(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr,
    void* buffer,
    size_t length,
    int flags
)
{
    socklen_t net_addr_length = sizeof(co_net_addr_t);

    ssize_t result = recvfrom(handle, buffer, (int)length, flags,
        (struct sockaddr*)net_addr, &net_addr_length);

    return result;
}

bool
co_socket_handle_set_option(
    co_socket_handle_t handle,
    int level,
    int name,
    const void* data,
    size_t length)
{
    int result = setsockopt(
        handle, level, name, data, (socklen_t)length);

    return (result == 0);
}

bool
co_socket_handle_get_option(
    co_socket_handle_t handle,
    int level,
    int name,
    void* buffer,
    size_t* length
)
{    
    int result = getsockopt(
        handle, level, name, buffer, (socklen_t*)length);

    return (result == 0);
}

bool
co_socket_handle_get_local_net_addr(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
)
{
    socklen_t net_add_length = sizeof(co_net_addr_t);

    int result = getsockname(
        handle, (struct sockaddr*)net_addr, &net_add_length);

    return (result == 0);
}

bool
co_socket_handle_get_remote_net_addr(
    co_socket_handle_t handle,
    co_net_addr_t* net_addr
)
{
    socklen_t net_add_length = sizeof(co_net_addr_t);

    int result = getpeername(
        handle, (struct sockaddr*)net_addr, &net_add_length);

    return (result == 0);
}

bool
co_socket_handle_set_blocking(
    co_socket_handle_t handle,
    bool enable
)
{
    u_long value = (enable ? 0 : 1);

    int result = ioctlsocket(handle, FIONBIO, &value);

    return (result == 0);
}