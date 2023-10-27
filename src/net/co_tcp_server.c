#include <coldforce/core/co_std.h>

#include <coldforce/net/co_tcp_server.h>
#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_net_log.h>

//---------------------------------------------------------------------------//
// tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

void
co_tcp_server_on_accept_ready(
    co_tcp_server_t* server
)
{
#ifdef CO_OS_WIN

    co_win_socket_option_update_accept_context(
        &server->sock, server->sock.win.server.accept.handle);

    co_tcp_client_t* win_client =
        co_tcp_client_create_with(
            server->sock.win.server.accept.handle, NULL);

    if (win_client == NULL)
    {
        co_socket_handle_close(
            server->sock.win.server.accept.handle);
        server->sock.win.server.accept.handle =
            CO_SOCKET_INVALID_HANDLE;

        return;
    }

    server->sock.win.server.accept.handle =
        CO_SOCKET_INVALID_HANDLE;

    co_tcp_log_info(
        &server->sock.local.net_addr,
        "<--",
        &win_client->sock.remote.net_addr,
        "tcp accept");

    if ((co_net_addr_get_family(
            &win_client->sock.remote.net_addr) != AF_UNSPEC) &&
        (server->callbacks.on_accept != NULL))
    {
        server->callbacks.on_accept(
            server->sock.owner_thread, server, win_client);
    }
    else
    {
        co_tcp_client_destroy(win_client);
    }

#endif

    for (;;)
    {
        if (!server->sock.local.is_open)
        {
            return;
        }

        co_net_addr_t remote_net_addr;

        co_socket_handle_t handle =
            co_socket_handle_accept(server->sock.handle, &remote_net_addr);

        if (handle == CO_SOCKET_INVALID_HANDLE)
        {
            break;
        }

        co_tcp_client_t* client =
            co_tcp_client_create_with(handle, &remote_net_addr);

        if (client == NULL)
        {
            co_socket_handle_close(handle);

            return;
        }

        co_tcp_log_info(
            &server->sock.local.net_addr,
            "<--",
            &client->sock.remote.net_addr,
            "tcp accept");

        if (server->callbacks.on_accept != NULL)
        {
            server->callbacks.on_accept(
                server->sock.owner_thread, server, client);
        }
        else
        {
            co_tcp_client_destroy(client);
        }
    }

#ifdef CO_OS_WIN
    co_win_net_accept_start(&server->sock);
#endif
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_tcp_server_t*
co_tcp_server_create(
    const co_net_addr_t* local_net_addr
)
{
    co_tcp_server_t* server =
        (co_tcp_server_t*)co_mem_alloc(sizeof(co_tcp_server_t));

    if (server == NULL)
    {
        return NULL;
    }

    co_socket_setup(
        &server->sock, CO_SOCKET_TYPE_TCP_SERVER);

    server->sock.owner_thread = co_thread_get_current();
    server->sock.local.is_open = true;

    memcpy(&server->sock.local.net_addr,
        local_net_addr, sizeof(co_net_addr_t));

    server->callbacks.on_accept = NULL;

#ifdef CO_OS_WIN

    if (!co_win_net_server_extension_setup(
        &server->sock.win.server, &server->sock))
    {
        co_mem_free(server);

        return NULL;
    }

    server->sock.handle = co_win_socket_handle_create_tcp(
        local_net_addr->sa.any.ss_family);
#else
    server->sock.handle = co_socket_handle_create(
        local_net_addr->sa.any.ss_family, SOCK_STREAM, 0);
#endif

    if (server->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
#ifdef CO_OS_WIN
        co_win_net_server_extension_cleanup(
            &server->sock.win.server);
#endif
        co_mem_free(server);

        return NULL;
    }

    return server;
}

void
co_tcp_server_destroy(
    co_tcp_server_t* server
)
{
    if (server != NULL)
    {
        co_tcp_server_close(server);
        co_mem_free(server);
    }
}

co_tcp_server_callbacks_st*
co_tcp_server_get_callbacks(
    co_tcp_server_t* server
)
{
    return &server->callbacks;
}

bool
co_tcp_server_start(
    co_tcp_server_t* server,
    int backlog
)
{
    if (server->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    if (!co_socket_handle_bind(
        server->sock.handle, &server->sock.local.net_addr))
    {
        return false;
    }

    if (!co_socket_handle_listen(server->sock.handle, backlog))
    {
        return false;
    }

    if (!co_net_worker_register_tcp_server(
        co_socket_get_net_worker(&server->sock), server))
    {
        return false;
    }

    co_socket_handle_get_local_net_addr(
        server->sock.handle, &server->sock.local.net_addr);

#ifdef CO_OS_WIN
    co_win_net_accept_start(&server->sock);
#endif

    co_tcp_log_info(
        &server->sock.local.net_addr,
        "tcp server start",
       NULL,
        "");

    return true;
}

void
co_tcp_server_close (
    co_tcp_server_t* server
)
{
    if (server == NULL)
    {
        return;
    }

    if (!server->sock.local.is_open)
    {
        return;
    }

    co_tcp_log_info(
        &server->sock.local.net_addr,
        "tcp server closed",
        NULL,
        "");

    co_net_worker_unregister_tcp_server(
        co_socket_get_net_worker(&server->sock),
        server);

#ifdef CO_OS_WIN
    co_win_net_server_extension_cleanup(
        &server->sock.win.server);
#endif

    server->callbacks.on_accept = NULL;

    co_socket_handle_close(server->sock.handle);
    server->sock.handle = CO_SOCKET_INVALID_HANDLE;
    server->sock.local.is_open = false;

    co_net_addr_remove_unix_path_file(
        &server->sock.local.net_addr);
}

bool
co_tcp_accept(
    co_thread_t* owner_thread,
    co_tcp_client_t* client
)
{
    if (co_thread_get_current() != owner_thread)
    {
        CO_DEBUG_SOCKET_COUNTER_DEC();

        return co_thread_send_event(owner_thread,
            CO_NET_EVENT_ID_TCP_ACCEPT_ON_THREAD, (uintptr_t)client, 0);
    }

    client->sock.owner_thread = owner_thread;

    if (!co_net_worker_register_tcp_connection(
        (co_net_worker_t*)owner_thread->event_worker, client))
    {
        return false;
    }

#ifdef CO_OS_WIN
    co_win_net_receive_start(&client->sock);
#endif

    return true;
}

co_socket_t*
co_tcp_server_get_socket(
    co_tcp_server_t* server
)
{
    return &server->sock;
}
