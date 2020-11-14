#include <coldforce/core/co_std.h>

#include <coldforce/net/co_tcp_server.h>
#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_event.h>

//---------------------------------------------------------------------------//
// tcp server
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_tcp_server_on_accept(
    co_tcp_server_t* server
)
{
    co_tcp_client_t* client =
        (co_tcp_client_t*)co_mem_alloc(sizeof(co_tcp_client_t));

    if (client == NULL)
    {
        return false;
    }

    if (!co_tcp_client_setup(client))
    {
        co_mem_free(client);

        return false;
    }

    client->sock.open_local = true;
    client->open_remote = true;

#ifdef CO_OS_WIN

    client->sock.handle = server->win.accept_handle;
    server->win.accept_handle = CO_SOCKET_INVALID_HANDLE;

    co_win_socket_option_update_accept_context(
        &client->sock, &server->sock);

    if (!co_win_tcp_server_accept_start(server))
    {
        co_tcp_client_destroy(client);

        return false;
    }

#else
    client->sock = CO_SocketAccept();
#endif

    co_socket_handle_get_local_net_addr(
        client->sock.handle, &client->sock.local_net_addr);

    co_socket_handle_get_remote_net_addr(
        client->sock.handle, &client->remote_net_addr);

    co_assert(server->on_accept != NULL);

    if (!server->on_accept(
        server->sock.owner_thread, server, client))
    {
        co_tcp_client_destroy(client);
    }

    return true;
}

//---------------------------------------------------------------------------//
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

    server->sock.owner_thread = co_thread_get_current();
    server->sock.open_local = true;
    server->on_accept = NULL;

    memcpy(&server->sock.local_net_addr,
        local_net_addr, sizeof(co_net_addr_t));

#ifdef CO_OS_WIN
    if (!co_win_tcp_server_setup(server))
    {
        co_mem_free(server);

        return NULL;
    }
#else
    server->sock.handle = co_socket_handle_create(
        local_net_addr->sa.any.ss_family, CO_SOCKET_TYPE_TCP, 0);
#endif

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

bool
co_tcp_server_start(
    co_tcp_server_t* server,
    co_tcp_accept_fn handler,
    int backlog
)
{
    if (server->sock.handle == CO_SOCKET_INVALID_HANDLE)
    {
        return false;
    }

    if (!co_socket_handle_bind(
        server->sock.handle, &server->sock.local_net_addr))
    {
        return false;
    }

    if (!co_socket_handle_listen(server->sock.handle, backlog))
    {
        return false;
    }

    if (!co_net_worker_register_tcp_server(
        (co_net_worker_t*)server->sock.owner_thread->event_worker, server))
    {
        return false;
    }

    co_socket_handle_get_local_net_addr(
        server->sock.handle, &server->sock.local_net_addr);

#ifdef CO_OS_WIN
    co_win_tcp_server_accept_start(server);
#endif

    server->on_accept = handler;

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

    if (!server->sock.open_local)
    {
        return;
    }

#ifdef CO_OS_WIN
    co_win_tcp_server_accept_stop(server);
#endif

    co_net_worker_unregister_tcp_server(
        (co_net_worker_t*)server->sock.owner_thread->event_worker,
        server);

#ifdef CO_OS_WIN
    co_win_tcp_server_cleanup(server);
#else
    co_socket_handle_close(server->sock.handle);
    server->sock.handle = CO_SOCKET_INVALID_HANDLE;
#endif

    server->sock.open_local = false;
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

bool
co_tcp_accept(
    co_thread_t* owner_thread,
    co_tcp_client_t* client
)
{
    if (co_thread_get_current() != owner_thread)
    {
        return co_event_send(owner_thread,
            CO_NET_EVENT_ID_TCP_HANDOVER, (uintptr_t)client, 0);
    }

    client->sock.owner_thread = owner_thread;

    if (!co_net_worker_register_tcp_connection(
        (co_net_worker_t*)owner_thread->event_worker, client))
    {
        return false;
    }

#ifdef CO_OS_WIN
    co_win_tcp_client_receive_start(client);
#endif

    return true;
}

void
co_tcp_set_handover_handler(
    co_thread_t* thread,
    co_tcp_handover_fn handler
)
{
    ((co_net_worker_t*)thread->event_worker)->on_tcp_handover = handler;
}
