#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_event.h>

//---------------------------------------------------------------------------//
// net worker
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

co_net_worker_t*
co_net_worker_create(
    void
)
{
    co_net_worker_t* net_worker =
        (co_net_worker_t*)co_mem_alloc(sizeof(co_net_worker_t));

    if (net_worker == NULL)
    {
        return NULL;
    }

    net_worker->event_worker.wait = (co_event_wait_fn)co_net_worker_wait;
    net_worker->event_worker.dispatch = (co_event_dispatch_fn)co_net_worker_dispatch;
    net_worker->event_worker.wake_up = (co_event_wake_up_fn)co_net_worker_wake_up;

    net_worker->net_selector = co_net_selector_create();
    net_worker->tcp_servers = NULL;
    net_worker->tcp_clients = NULL;
    net_worker->udps = NULL;

    net_worker->on_tcp_handover = NULL;

#ifdef CO_DEBUG
    net_worker->sock_counter = 0;
#endif

    return net_worker;
}

void
co_net_worker_cleanup(
    co_net_worker_t* net_worker
)
{
    if (net_worker->tcp_servers != NULL)
    {
        co_list_destroy(net_worker->tcp_servers);
        net_worker->tcp_servers = NULL;
    }

    if (net_worker->tcp_clients != NULL)
    {
        if (co_list_get_size(net_worker->tcp_clients) > 0)
        {
            for (int counter = 0; counter < 3; ++counter)
            {
                while (net_worker->event_worker.wait(
                    &net_worker->event_worker, 1000) != CO_WAIT_RESULT_TIMEOUT)
                {
                    ;
                }

                if (co_list_get_size(
                    net_worker->tcp_clients) == 0)
                {
                    break;
                }
            }

            while (co_list_get_size(net_worker->tcp_clients) > 0)
            {
                co_list_data_st* data =
                    co_list_get_head(net_worker->tcp_clients);

                co_tcp_client_on_close((co_tcp_client_t*)data->value);
            }
        }

        co_list_destroy(net_worker->tcp_clients);
        net_worker->tcp_clients = NULL;
    }

    if (net_worker->udps != NULL)
    {
        co_list_destroy(net_worker->udps);
        net_worker->udps = NULL;
    }

    co_net_selector_destroy(net_worker->net_selector);
    net_worker->net_selector = NULL;

    co_assert(net_worker->sock_counter == 0);
}

co_wait_result_t
co_net_worker_wait(
    co_net_worker_t* net_worker,
    uint32_t msec
)
{
    return co_net_selector_wait(net_worker->net_selector, msec);
}

void
co_net_worker_wake_up(
    co_net_worker_t* net_worker
)
{
    co_net_selector_wake_up(net_worker->net_selector);
}

bool
co_net_worker_dispatch(
    co_net_worker_t* net_worker,
    co_event_t* event
)
{
    switch (event->event_id)
    {
    case CO_NET_EVENT_ID_TCP_ACCEPT:
    {
        co_tcp_server_on_accept(
            (co_tcp_server_t*)event->param1);
        break;
    }
    case CO_NET_EVENT_ID_TCP_CONNECT:
    {
        co_tcp_client_on_connect(
            (co_tcp_client_t*)event->param1, (int)event->param2);
        break;
    }
    case CO_NET_EVENT_ID_TCP_SEND:
    {
        co_tcp_client_on_send(
            (co_tcp_client_t*)event->param1, (size_t)event->param2);
        break;
    }
    case CO_NET_EVENT_ID_TCP_RECEIVE:
    {
        co_tcp_client_on_receive(
            (co_tcp_client_t*)event->param1, (size_t)event->param2);
        break;
    }
    case CO_NET_EVENT_ID_TCP_CLOSE:
    {
        co_tcp_client_on_close(
            (co_tcp_client_t*)event->param1);
        break;
    }
    case CO_NET_EVENT_ID_TCP_HANDOVER:
    {
        co_thread_t* thread = co_thread_get_current();
        co_tcp_client_t* client = (co_tcp_client_t*)event->param1;

        net_worker->on_tcp_handover(thread, client);

        break;
    }
    case CO_NET_EVENT_ID_UDP_SEND:
    {
        co_udp_on_send(
            (co_udp_t*)event->param1, (size_t)event->param2);

        break;
    }
    case CO_NET_EVENT_ID_UDP_RECEIVE:
    {
        co_udp_on_receive(
            (co_udp_t*)event->param1, (size_t)event->param2);

        break;
    }
    default:
        return co_event_worker_dispatch(&net_worker->event_worker, event);
    }

    return true;
}

bool
co_net_worker_register_tcp_server(
    co_net_worker_t* net_worker,
    co_tcp_server_t* server
)
{
    if (!co_net_selector_register(
        net_worker->net_selector, &server->sock))
    {
        return false;
    }

    if (net_worker->tcp_servers == NULL)
    {
        net_worker->tcp_servers = co_list_create(NULL);
    }

    co_list_add_tail(
        net_worker->tcp_servers, (uintptr_t)server);

    return true;
}

void
co_net_worker_unregister_tcp_server(
    co_net_worker_t* net_worker,
    co_tcp_server_t* server
)
{
    if (net_worker->tcp_servers == NULL)
    {
        return;
    }

    co_list_iterator_t* it =
        co_list_find(net_worker->tcp_servers, (uintptr_t)server);

    if (it == NULL)
    {
        return;
    }

    co_list_remove_at(net_worker->tcp_servers, it);

    co_net_selector_unregister(
        net_worker->net_selector, &server->sock);
}

bool
co_net_worker_register_tcp_connector(
    co_net_worker_t* net_worker,
    co_tcp_client_t* client
)
{
    if (net_worker->tcp_clients == NULL)
    {
        net_worker->tcp_clients = co_list_create(NULL);
    }

    if (co_list_contains(
        net_worker->tcp_clients, (uintptr_t)client))
    {
        return true;
    }

    if (!co_net_selector_register(
        net_worker->net_selector, &client->sock))
    {
        return false;
    }

    co_list_add_tail(
        net_worker->tcp_clients, (uintptr_t)client);

    return true;
}

void
co_net_worker_unregister_tcp_connector(
    co_net_worker_t* net_worker,
    co_tcp_client_t* client
)
{
    if (net_worker->tcp_clients == NULL)
    {
        return;
    }

    co_list_iterator_t* it =
        co_list_find(net_worker->tcp_clients, (uintptr_t)client);

    if (it == NULL)
    {
        return;
    }

    co_list_remove_at(net_worker->tcp_clients, it);

    co_net_selector_unregister(
        net_worker->net_selector, &client->sock);
}

bool
co_net_worker_register_tcp_connection(
    co_net_worker_t* net_worker,
    co_tcp_client_t* client
)
{
    if (net_worker->tcp_clients == NULL)
    {
        net_worker->tcp_clients = co_list_create(NULL);
    }

    if (co_list_contains(
        net_worker->tcp_clients, (uintptr_t)client))
    {
        return true;
    }

    if (!co_net_selector_register(
        net_worker->net_selector, &client->sock))
    {
        return false;
    }

    co_list_add_tail(
        net_worker->tcp_clients, (uintptr_t)client);

    return true;
}

void
co_net_worker_unregister_tcp_connection(
    co_net_worker_t* net_worker,
    co_tcp_client_t* client)
{
    if (net_worker->tcp_clients != NULL)
    {
        co_list_iterator_t* it =
            co_list_find(net_worker->tcp_clients, (uintptr_t)client);

        if (it != NULL)
        {
            if (client->close_timer != NULL)
            {
                co_timer_stop(client->close_timer);
                co_timer_destroy(client->close_timer);

                client->close_timer = NULL;
            }

            co_list_remove_at(net_worker->tcp_clients, it);

            co_net_selector_unregister(net_worker->net_selector, &client->sock);
        }
    }

    co_socket_handle_close(client->sock.handle);
    client->sock.handle = CO_SOCKET_INVALID_HANDLE;
}

void
co_net_worker_tcp_client_close_timer(
    co_thread_t* thread,
    co_timer_t* timer
)
{
    co_tcp_client_t* client =
        (co_tcp_client_t*)co_timer_get_param(timer);

    client->open_remote = false;

    co_net_worker_unregister_tcp_connection(
        (co_net_worker_t*)thread->event_worker, client);

    co_tcp_client_destroy(client);
}

void
co_net_worker_close_tcp_client_local(
    co_net_worker_t* net_worker,
    co_tcp_client_t* client
)
{
    if (!client->sock.open_local)
    {
        return;
    }

    client->sock.open_local = false;

    if (client->open_remote)
    {
        co_socket_handle_shutdown(
            client->sock.handle, CO_SOCKET_SHUTDOWN_SEND);

        client->close_timer = co_timer_create(3000,
            (co_timer_fn)co_net_worker_tcp_client_close_timer,
            false, (uintptr_t)client);
        
        co_timer_start(client->close_timer);
    }
    else
    {
        co_net_worker_unregister_tcp_connection(net_worker, client);
    }
}

void
co_net_worker_close_tcp_client_remote(
    co_net_worker_t* net_worker,
    co_tcp_client_t* client
)
{
    if (!client->open_remote)
    {
        return;
    }

    client->open_remote = false;

    co_net_worker_unregister_tcp_connection(net_worker, client);
}

bool
co_net_worker_register_udp(
    co_net_worker_t* net_worker,
    co_udp_t* udp
)
{
    if (!co_net_selector_register(
        net_worker->net_selector, &udp->sock))
    {
        return false;
    }

    if (net_worker->udps == NULL)
    {
        net_worker->udps = co_list_create(NULL);
    }

    co_list_add_tail(
        net_worker->udps, (uintptr_t)udp);

    return true;
}

void
co_net_worker_unregister_udp(
    co_net_worker_t* net_worker,
    co_udp_t* udp
)
{
    if (net_worker->udps == NULL)
    {
        return;
    }

    co_list_iterator_t* it =
        co_list_find(net_worker->udps, (uintptr_t)udp);

    if (it == NULL)
    {
        return;
    }

    co_list_remove_at(net_worker->udps, it);

    co_net_selector_unregister(
        net_worker->net_selector, &udp->sock);
}

