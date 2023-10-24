#ifndef CO_SOCKET_OPTION_H_INCLUDED
#define CO_SOCKET_OPTION_H_INCLUDED

#include <coldforce/net/co_net.h>
#include <coldforce/net/co_socket.h>

CO_EXTERN_C_BEGIN

//---------------------------------------------------------------------------//
// socket option
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

CO_NET_API
bool
co_socket_option_set(
    co_socket_t* sock,
    int level,
    int name,
    const void* data,
    size_t data_size
);

CO_NET_API
bool
co_socket_option_get(
    const co_socket_t* sock,
    int level,
    int name,
    void* buffer,
    size_t* buffer_size
);

// SO_REUSEADDR

CO_NET_API
bool
co_socket_option_set_reuse_addr(
    co_socket_t* sock,
    bool enable
);

CO_NET_API
bool
co_socket_option_get_reuse_addr(
    const co_socket_t* sock,
    bool* enable
);

// SO_KEEPALIVE

CO_NET_API
bool
co_socket_option_set_keep_alive(
    co_socket_t* sock,
    bool enable
);

CO_NET_API
bool
co_socket_option_get_keep_alive(
    const co_socket_t* sock,
    bool* enable
);

// SO_SNDBUF

CO_NET_API
bool
co_socket_option_set_send_buffer(
    co_socket_t* sock,
    size_t buffer_size
);

CO_NET_API
bool
co_socket_option_get_send_buffer(
    const co_socket_t* sock,
    size_t* buffer_size
);

// SO_RCVBUF

CO_NET_API
bool
co_socket_option_set_receive_buffer(
    co_socket_t* sock,
    size_t buffer_size
);

CO_NET_API
bool
co_socket_option_get_receive_buffer(
    const co_socket_t* sock,
    size_t* buffer_size
);

// SO_LINGER

CO_NET_API
bool
co_socket_option_set_linger(
    co_socket_t* sock,
    const struct linger* linger
);

CO_NET_API
bool
co_socket_option_get_linger(
    const co_socket_t* sock,
    struct linger* linger
);

// SO_ERROR

CO_NET_API
bool
co_socket_option_get_error(
    const co_socket_t* sock,
    int* error_code
);

// TCP_NODELAY

CO_NET_API
bool
co_socket_option_set_tcp_no_delay(
    co_socket_t* sock,
    bool enable
);

CO_NET_API
bool
co_socket_option_get_tcp_no_delay(
    const co_socket_t* sock,
    bool* enable
);

// SO_REUSEPORT

#ifdef SO_REUSEPORT

CO_NET_API
bool
co_socket_option_set_reuse_port(
    co_socket_t* sock,
    bool enable
);

CO_NET_API
bool
co_socket_option_get_reuse_port(
    const co_socket_t* sock,
    bool* enable
);

#endif // SO_REUSEPORT

// IP_ADD_MEMBERSHIP

CO_NET_API
bool
co_socket_option_set_add_membership(
    co_socket_t* sock,
    const struct ip_mreq* mreq
);

CO_NET_API
bool
co_socket_option_get_add_membership(
    const co_socket_t* sock,
    struct ip_mreq* mreq
);

// SO_BROADCAST

CO_NET_API
bool
co_socket_option_set_broadcast(
    co_socket_t* sock,
    bool enable
);

CO_NET_API
bool
co_socket_option_get_broadcast(
    const co_socket_t* sock,
    bool* enable
);

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

CO_EXTERN_C_END

#endif // CO_SOCKET_OPTION_H_INCLUDED
