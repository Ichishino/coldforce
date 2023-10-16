#ifndef CO_NET_WIN_H_INCLUDED
#define CO_NET_WIN_H_INCLUDED

#include <coldforce/core/co_list.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_socket_handle.h>

#ifdef CO_OS_WIN

#include <coldforce/net/co_net_selector_win.h>

CO_EXTERN_C_BEGIN

struct co_socket_t;

//---------------------------------------------------------------------------//
// network (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WIN_TCP_DEFAULT_RECEIVE_BUFFER_SIZE      8192
#define CO_WIN_UDP_DEFAULT_RECEIVE_BUFFER_SIZE      65536

typedef struct
{
    co_list_t* io_ctxs;

    struct co_win_net_receive_ctx_t
    {
        co_win_net_io_ctx_t* io_ctx;

        WSABUF buffer;
        size_t size;
        size_t index;

        size_t new_size;
        co_net_addr_t* remote_net_addr;

    } receive;

} co_win_net_client_extesion_t;

typedef struct
{
    struct co_win_net_accept_ctx_t
    {
        co_win_net_io_ctx_t* io_ctx;
        co_socket_handle_t handle;
        uint8_t* buffer;

    } accept;

} co_win_net_server_extension_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_win_net_setup(
    void
);

void
co_win_net_cleanup(
    void
);

bool
co_win_net_client_extension_setup(
    co_win_net_client_extesion_t* win_client,
    struct co_socket_t* sock,
    size_t receive_buffer_size
);

void
co_win_net_client_extension_cleanup(
    co_win_net_client_extesion_t* win_client
);

bool
co_win_net_server_extension_setup(
    co_win_net_server_extension_t* win,
    struct co_socket_t* sock
);

void
co_win_net_server_extension_cleanup(
    co_win_net_server_extension_t* win
);

bool
co_win_net_connector_extension_setup(
    co_win_net_client_extesion_t* win_client,
    struct co_socket_t* sock
);

co_socket_handle_t
co_win_socket_handle_create_tcp(
    co_net_addr_family_t family
);

co_socket_handle_t
co_win_socket_handle_create_udp(
    co_net_addr_family_t family
);

bool
co_win_net_accept_start(
    struct co_socket_t* sock
);

bool
co_win_net_connect_start(
    struct co_socket_t* sock,
    const co_net_addr_t* remote_net_addr
);

bool
co_win_net_send(
    struct co_socket_t* sock,
    const void* data,
    size_t data_size
);

ssize_t
co_win_net_send_async(
    struct co_socket_t* sock,
    const void* data,
    size_t data_size
);

bool
co_win_net_receive_start(
    struct co_socket_t* sock
);

ssize_t
co_win_net_receive(
    struct co_socket_t* sock,
    void* buffer,
    size_t buffer_size
);

bool
co_win_net_send_to(
    struct co_socket_t* sock,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
);

ssize_t
co_win_net_send_to_async(
    struct co_socket_t* sock,
    const co_net_addr_t* remote_net_addr,
    const void* data,
    size_t data_size
);

bool
co_win_net_receive_from_start(
    struct co_socket_t* sock
);

ssize_t
co_win_net_receive_from(
    struct co_socket_t* sock,
    co_net_addr_t* remote_net_addr,
    void* buffer,
    size_t buffer_size
);

bool
co_win_socket_option_update_connect_context(
    struct co_socket_t* sock
);

bool
co_win_socket_option_update_accept_context(
    struct co_socket_t* server_sock,
    co_socket_handle_t accept_handle
);

bool
co_win_socket_option_get_connect_time(
    struct co_socket_t* sock,
    int* seconds
);

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
size_t
co_win_socket_get_receive_data_size(
    const struct co_socket_t* sock
);

CO_NET_API
void
co_win_socket_set_receive_buffer_size(
    struct co_socket_t* sock,
    size_t new_size
);

CO_NET_API
size_t
co_win_socket_get_receive_buffer_size(
    const struct co_socket_t* sock
);

CO_NET_API
const void*
co_win_socket_get_receive_buffer(
    const struct co_socket_t* sock
);

CO_NET_API
void
co_win_socket_clear_receive_buffer(
    struct co_socket_t* sock
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_WIN

#endif // CO_NET_H_INCLUDED
