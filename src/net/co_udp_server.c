#include <coldforce/core/co_std.h>

#include <coldforce/net/co_udp_server.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_log.h>
#include <coldforce/net/co_socket_option.h>

//---------------------------------------------------------------------------//
// udp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_UDP_ACCEPT_DATA_SIZE     8192

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

static void
co_udp_server_on_udp_receive(
    co_thread_t* thread,
    co_udp_server_t* udp_server
)
{
    for (;;)
    {
        co_net_addr_t remote_net_addr = { 0 };

        uint8_t buffer[CO_UDP_ACCEPT_DATA_SIZE];

        ssize_t data_size = co_udp_receive_from(
            &udp_server->udp, &remote_net_addr, buffer, sizeof(buffer));

        if (data_size <= 0)
        {
            break;
        }

        if (udp_server->callbacks.on_accept == NULL)
        {
            break;
        }

        co_udp_t* udp_conn =
            co_udp_create_connection(
                udp_server, &remote_net_addr, buffer, data_size);

        if (udp_conn == NULL)
        {
            break;
        }

        udp_server->callbacks.on_accept(thread, udp_server, udp_conn);
    }
}

co_udp_t*
co_udp_create_connection(
    const co_udp_server_t* udp_server,
    const co_net_addr_t* remote_net_addr,
    const uint8_t* data,
    size_t data_size
)
{
    co_socket_handle_t handle =
        co_socket_handle_create(
            udp_server->udp.sock.local.net_addr.sa.any.ss_family,
            SOCK_DGRAM, 0);

    if (handle == CO_SOCKET_INVALID_HANDLE)
    {
        return NULL;
    }

    co_udp_connection_t* udp_conn =
        (co_udp_connection_t*)co_mem_alloc(sizeof(co_udp_connection_t));

    if (udp_conn == NULL)
    {
        return NULL;
    }

    co_udp_t* udp = &udp_conn->udp;

    if (!co_udp_setup(udp, CO_SOCKET_TYPE_UDP))
    {
        co_udp_cleanup(udp);
        co_mem_free(udp_conn);

        return NULL;
    }

    udp->sock.handle = handle;
    udp->sock.local.is_open = true;

    memcpy(&udp->sock.local.net_addr,
        &udp_server->udp.sock.local.net_addr, sizeof(co_net_addr_t));
    memcpy(&udp->sock.remote.net_addr,
        remote_net_addr, sizeof(co_net_addr_t));

    udp_conn->accept_data.ptr = (uint8_t*)co_mem_alloc(data_size);

    if (udp_conn->accept_data.ptr == NULL)
    {
        co_udp_cleanup(udp);
        co_mem_free(udp_conn);

        return NULL;
    }

    memcpy(udp_conn->accept_data.ptr, data, data_size);
    udp_conn->accept_data.size = data_size;

    return udp;
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_udp_server_t*
co_udp_server_create(
    const co_net_addr_t* local_net_addr
)
{
    co_udp_server_t* udp_server =
        (co_udp_server_t*)co_mem_alloc(sizeof(co_udp_server_t));

    if (udp_server == NULL)
    {
        return NULL;
    }

    if (!co_udp_setup_new_handle(&udp_server->udp, local_net_addr))
    {
        co_mem_free(udp_server);

        return NULL;
    }

    udp_server->udp.callbacks.on_receive =
        (co_udp_receive_fn)co_udp_server_on_udp_receive;

    udp_server->callbacks.on_accept = NULL;

    return udp_server;
}

void
co_udp_server_destroy(
    co_udp_server_t* udp_server
)
{
    co_udp_destroy(&udp_server->udp);
}

bool
co_udp_server_start(
    co_udp_server_t* udp_server
)
{
    return co_udp_receive_start(&udp_server->udp);
}

bool
co_udp_accept(
    co_thread_t* owner_thread,
    co_udp_t* udp_conn
)
{
    if (co_thread_get_current() != owner_thread)
    {       
        if (!co_thread_send_event(owner_thread,
            CO_NET_EVENT_ID_UDP_ACCEPT_ON_THREAD,
            (uintptr_t)udp_conn, 0))
        {
            co_udp_destroy(udp_conn);
                       
            return false;
        }

        CO_DEBUG_SOCKET_COUNTER_DEC();

        return true;
    }

    co_udp_log_info(
        &udp_conn->sock.local.net_addr,
        "<--",
        &udp_conn->sock.remote.net_addr,
        "udp accept");

    udp_conn->sock.owner_thread = owner_thread;

    co_socket_option_set_reuse_addr(&udp_conn->sock, true);

    if (!co_udp_connect(
        udp_conn, &udp_conn->sock.remote.net_addr))
    {
        return false;
    }

    if (!co_udp_receive_start(udp_conn))
    {
        return false;
    }

    return true;
}

size_t
co_udp_get_accept_data(
    const co_udp_t* udp_conn,
    const uint8_t** buffer
)
{
    co_udp_connection_t* conn =
        (co_udp_connection_t*)udp_conn;

    *buffer = conn->accept_data.ptr;

    return conn->accept_data.size;
}

void
co_udp_destroy_connection(
    co_udp_t* udp_conn
)
{
    if (udp_conn != NULL)
    {
        co_udp_connection_t* conn =
            (co_udp_connection_t*)udp_conn;

        co_mem_free(conn->accept_data.ptr);
        conn->accept_data.ptr = NULL;

        co_udp_destroy(udp_conn);
    }
}

co_udp_server_callbacks_st*
co_udp_server_get_callbacks(
    co_udp_server_t* udp_server
)
{
    return &udp_server->callbacks;
}

co_socket_t*
co_udp_server_get_socket(
    co_udp_server_t* udp_server
)
{
    return &udp_server->udp.sock;
}
