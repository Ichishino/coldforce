#ifndef CO_SOCKET_HANDLE_H_INCLUDED
#define CO_SOCKET_HANDLE_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// socket handle
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#ifdef CO_OS_WIN

typedef SOCKET co_socket_handle_t;
#define CO_SOCKET_INVALID_HANDLE    ((co_socket_handle_t)INVALID_SOCKET)

typedef enum
{
    CO_SOCKET_SHUTDOWN_SEND = SD_SEND,
    CO_SOCKET_SHUTDOWN_BOTH = SD_BOTH

} co_socket_shutdown_how_t;

#define CO_TCP_SEND_FLAGS   0

#elif defined(CO_OS_LINUX)

typedef int32_t co_socket_handle_t;
#define CO_SOCKET_INVALID_HANDLE    (-1)

typedef enum
{
    CO_SOCKET_SHUTDOWN_SEND = SHUT_WR,
    CO_SOCKET_SHUTDOWN_BOTH = SHUT_RDWR

} co_socket_shutdown_how_t;

#define CO_TCP_SEND_FLAGS   MSG_NOSIGNAL

#elif defined(CO_OS_MAC)

typedef int32_t co_socket_handle_t;
#define CO_SOCKET_INVALID_HANDLE    (-1)

typedef enum
{
    CO_SOCKET_SHUTDOWN_SEND = SHUT_WR,
    CO_SOCKET_SHUTDOWN_BOTH = SHUT_RDWR

} co_socket_shutdown_how_t;

#define CO_TCP_SEND_FLAGS   0

#endif

typedef enum
{
    CO_SOCKET_TYPE_TCP = SOCK_STREAM,
    CO_SOCKET_TYPE_UDP = SOCK_DGRAM

} co_socket_type_t;

typedef enum
{
    CO_PROTOCOL_IP = IPPROTO_IP,
    CO_PROTOCOL_TCP = IPPROTO_TCP,
    CO_PROTOCOL_UDP = IPPROTO_UDP

} co_protocol_t;

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_socket_handle_t co_socket_handle_create(
    co_address_family_t family, co_socket_type_t type, co_protocol_t protocol);

void co_socket_handle_close(co_socket_handle_t handle);

bool co_socket_handle_shutdown(
    co_socket_handle_t handle, co_socket_shutdown_how_t how);

bool co_socket_handle_bind(
    co_socket_handle_t handle, const co_net_addr_t* net_addr);

bool co_socket_handle_listen(co_socket_handle_t handle, int backlog);

co_socket_handle_t co_socket_handle_accept(
    co_socket_handle_t handle, co_net_addr_t* net_addr);

bool co_socket_handle_connect(
    co_socket_handle_t handle, const co_net_addr_t* net_addr);

ssize_t co_socket_handle_send(
    co_socket_handle_t handle, const void* data, size_t length, int flags);

ssize_t co_socket_handle_receive(
    co_socket_handle_t handle, void* buffer, size_t length, int flags);

ssize_t co_socket_handle_send_to(co_socket_handle_t handle,
    const co_net_addr_t* net_addr, const void* data, size_t length, int flags);

ssize_t co_socket_handle_receive_from(co_socket_handle_t handle,
    co_net_addr_t* net_addr, void* buffer, size_t length, int flags);

bool co_socket_handle_set_option(co_socket_handle_t handle,
    int level, int name, const void* data, size_t length);

bool co_socket_handle_get_option(co_socket_handle_t handle,
    int level, int name, void* buffer, size_t* length);

bool co_socket_handle_get_local_net_addr(
    co_socket_handle_t handle, co_net_addr_t* net_addr);

bool co_socket_handle_get_remote_net_addr(
    co_socket_handle_t handle, co_net_addr_t* net_addr);

bool co_socket_handle_set_blocking(co_socket_handle_t handle, bool enable);

int co_socket_get_error(void);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SOCKET_HANDLE_H_INCLUDED
