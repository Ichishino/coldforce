#ifndef CO_TCP_WIN_H_INCLUDED
#define CO_TCP_WIN_H_INCLUDED

#include <coldforce/core/co_list.h>

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_net_addr.h>
#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_socket_handle.h>
#include <coldforce/net/co_socket.h>

#ifdef CO_OS_WIN

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// tcp (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WIN_TCP_DEFAULT_RECEIVE_BUFFER_SIZE  8192

struct co_tcp_server_t;
struct co_tcp_client_t;

typedef struct
{
    co_win_net_io_ctx_t* accept_io_ctx;
    co_socket_handle_t accept_handle;
    uint8_t buffer[512];

} co_win_tcp_server_extention_t;

typedef struct
{
    co_win_net_io_ctx_t* io_connect_ctx;
    co_list_t* io_send_ctxs;

    struct co_tcp_receive_ctx_t
    {
        co_win_net_io_ctx_t* io_ctx;

        WSABUF buffer;
        size_t size;
        size_t index;

        size_t new_size;

    } receive;

} co_win_tcp_client_extension_t;

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

bool
co_win_tcp_load_functions(
    void
);

co_socket_handle_t
co_win_tcp_socket_create(
    co_address_family_t family
);

bool
co_win_tcp_server_setup(
    struct co_tcp_server_t* server
);

void
co_win_tcp_server_cleanup(
    struct co_tcp_server_t* server
);

bool
co_win_tcp_server_accept_start(
    struct co_tcp_server_t* server
);

void
co_win_tcp_server_accept_stop(
    struct co_tcp_server_t* server
);

bool
co_win_tcp_client_setup(
    struct co_tcp_client_t* client,
    size_t receive_buffer_size
);

void
co_win_tcp_client_cleanup(
    struct co_tcp_client_t* client
);

bool
co_win_tcp_client_connector_setup(
    struct co_tcp_client_t* client,
    const co_net_addr_t* local_net_addr
);

void
co_win_tcp_client_connector_cleanup(
    struct co_tcp_client_t* client
);

bool
co_win_tcp_client_send(
    struct co_tcp_client_t* client,
    const void* data,
    size_t data_size
);

bool
co_win_tcp_client_send_async(
    struct co_tcp_client_t* client,
    const void* data,
    size_t data_size
);

bool
co_win_tcp_client_receive_start(
    struct co_tcp_client_t* client
);

bool
co_win_tcp_client_receive(
    struct co_tcp_client_t* client,
    void* buffer,
    size_t buffer_size,
    size_t* data_size
);

bool
co_win_tcp_client_connect_start(
    struct co_tcp_client_t* client,
    const co_net_addr_t* remote_net_addr
);

bool
co_win_socket_option_update_connect_context(
    co_socket_t* sock
);

bool
co_win_socket_option_update_accept_context(
    co_socket_t* sock,
    co_socket_t* server_sock
);

bool
co_win_socket_option_get_connect_time(
    co_socket_t* sock,
    int* seconds
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_OS_WIN

#endif // CO_TCP_WIN_H_INCLUDED
