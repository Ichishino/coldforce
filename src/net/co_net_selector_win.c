#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_net_worker.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_socket.h>
#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_tcp_win.h>

#ifdef CO_OS_WIN

#include <mswsock.h>

//---------------------------------------------------------------------------//
// net selector (windows)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

#define CO_WIN_NET_IOCP_COMP_KEY_CANCEL     1

co_net_selector_t*
co_net_selector_create(
    void
)
{
    co_net_selector_t* net_selector =
        (co_net_selector_t*)co_mem_alloc(sizeof(co_net_selector_t));

    if (net_selector == NULL)
    {
        return NULL;
    }

    net_selector->iocp =
        CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    if (net_selector->iocp == NULL)
    {
        co_mem_free(net_selector);

        return NULL;
    }

    net_selector->ol_entries =
        co_array_create(sizeof(OVERLAPPED_ENTRY));

    net_selector->sock_count = 0;
    co_array_set_count(net_selector->ol_entries, 1);

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.free_value = (co_item_free_fn)co_mem_free;
    net_selector->io_ctx_trash = co_list_create(&list_ctx);

    return net_selector;
}

void
co_net_selector_destroy(
    co_net_selector_t* net_selector
)
{
    co_assert(net_selector->sock_count == 0);

    co_list_destroy(net_selector->io_ctx_trash);
    net_selector->io_ctx_trash = NULL;

    co_array_destroy(net_selector->ol_entries);

    CloseHandle(net_selector->iocp);

    co_mem_free(net_selector);
}

bool
co_net_selector_register(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags
)
{
    (void)flags;

    if (CreateIoCompletionPort(
        (HANDLE)sock->handle, (HANDLE)net_selector->iocp, 0, 0) == NULL)
    {
        return false;
    }

    co_socket_handle_set_blocking(sock->handle, false);

    ++net_selector->sock_count;

    return true;
}

void
co_net_selector_unregister(
    co_net_selector_t* net_selector,
    co_socket_t* sock
)
{
    co_assert(net_selector->sock_count > 0);

    CancelIo((HANDLE)sock->handle);

    co_socket_handle_set_blocking(sock->handle, true);

    --net_selector->sock_count;
}

bool
co_net_selector_update(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags)
{
    (void)net_selector;
    (void)sock;
    (void)flags;

    return true;
}

co_wait_result_t
co_net_selector_wait(
    co_net_selector_t* net_selector,
    uint32_t msec
)
{
    co_array_set_count(
        net_selector->ol_entries, net_selector->sock_count + 1);
    co_array_zero_clear(net_selector->ol_entries);

    LPOVERLAPPED_ENTRY entries =
        (LPOVERLAPPED_ENTRY)co_array_get_ptr(net_selector->ol_entries, 0);

    ULONG count = (ULONG)co_array_get_count(net_selector->ol_entries);
    ULONG removed = 0;

    if (GetQueuedCompletionStatusEx(
        net_selector->iocp, entries, count, &removed, msec, FALSE))
    {
        for (ULONG index = 0; index < removed; ++index)
        {
            if (entries[index].lpCompletionKey == CO_WIN_NET_IOCP_COMP_KEY_CANCEL)
            {
                continue;
            }

            co_win_net_io_ctx_t* io_ctx =
                (co_win_net_io_ctx_t*)entries[index].lpOverlapped;

            switch (io_ctx->id)
            {
            case CO_WIN_NET_IO_ID_TCP_RECEIVE:
            {
                if (io_ctx->sock == NULL)
                {
                    break;
                }

                size_t data_length =
                    (size_t)entries[index].dwNumberOfBytesTransferred;

                if (data_length > 0)
                {
                    if (!co_thread_send_event(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_RECEIVE_READY,
                        (uintptr_t)io_ctx->sock, (uintptr_t)data_length))
                    {
                        co_tcp_client_on_receive_ready(
                            (co_tcp_client_t*)io_ctx->sock, data_length);
                    }
                }
                else
                {
                    if (!co_thread_send_event(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_CLOSE,
                        (uintptr_t)io_ctx->sock, 0))
                    {
                        co_tcp_client_on_close((co_tcp_client_t*)io_ctx->sock);
                    }
                }

                break;
            }
            case CO_WIN_NET_IO_ID_TCP_SEND:
            {
                if (io_ctx->sock != NULL)
                {
                    co_thread_send_event(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_SEND_COMPLETE,
                        (uintptr_t)io_ctx->sock,
                        (uintptr_t)entries[index].dwNumberOfBytesTransferred);
                }

                break;
            }
            case CO_WIN_NET_IO_ID_TCP_ACCEPT:
            {
                if (io_ctx->sock != NULL)
                {
                    co_thread_send_event(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_ACCEPT_READY,
                        (uintptr_t)io_ctx->sock,
                        0);
                }

                break;
            }
            case CO_WIN_NET_IO_ID_TCP_CONNECT:
            {
                if (io_ctx->sock == NULL)
                {
                    break;
                }

                int error_code = CO_NET_ERROR_TCP_CONNECT_FAILED;
                int seconds = -1;

                if (co_win_socket_option_get_connect_time(
                    io_ctx->sock, &seconds))
                {
                    if (seconds != -1)
                    {
                        error_code = 0;
                    }
                }

                co_thread_send_event(
                    io_ctx->sock->owner_thread,
                    CO_NET_EVENT_ID_TCP_CONNECT_COMPLETE,
                    (uintptr_t)io_ctx->sock,
                    (uintptr_t)error_code);

                break;
            }
            case CO_WIN_NET_IO_ID_UDP_SEND:
            {
                if (io_ctx->sock != NULL)
                {
                    co_thread_send_event(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_UDP_SEND_COMPLETE,
                        (uintptr_t)io_ctx->sock,
                        (uintptr_t)entries[index].dwNumberOfBytesTransferred);
                }

                break;
            }
            case CO_WIN_NET_IO_ID_UDP_RECEIVE:
            {
                if (io_ctx->sock != NULL)
                {
                    co_thread_send_event(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_UDP_RECEIVE_READY,
                        (uintptr_t)io_ctx->sock,
                        (uintptr_t)entries[index].dwNumberOfBytesTransferred);
                }

                break;
            }
            default:
                co_assert(false);
                break;
            }
        }
    }
    else
    {
        int error = co_socket_get_error();

        if (error == WAIT_TIMEOUT)
        {
            return CO_WAIT_RESULT_TIMEOUT;
        }
        else
        {
            return CO_WAIT_RESULT_ERROR;
        }
    }

    return CO_WAIT_RESULT_SUCCESS;
}

void
co_net_selector_wake_up(
    co_net_selector_t* net_selector
)
{
    PostQueuedCompletionStatus(
        net_selector->iocp, 0, CO_WIN_NET_IOCP_COMP_KEY_CANCEL, NULL);
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

void
co_win_free_io_ctx(
    co_win_net_io_ctx_t* io_ctx
)
{
    if (HasOverlappedIoCompleted((LPOVERLAPPED)io_ctx))
    {
        co_mem_free(io_ctx);
    }
    else
    {
        co_list_add_tail(
            co_socket_get_net_worker(
                io_ctx->sock)->net_selector->io_ctx_trash,
            (uintptr_t)io_ctx);

        io_ctx->sock = NULL;
    }
}

void
co_win_try_clear_io_ctx_trash(
    co_net_selector_t* net_selector
)
{
    co_list_iterator_t* it =
        co_list_get_head_iterator(net_selector->io_ctx_trash);

    while (it != NULL)
    {
        co_list_data_st* data =
            co_list_get(net_selector->io_ctx_trash, it);

        if (HasOverlappedIoCompleted((LPOVERLAPPED)data->value))
        {
            co_list_iterator_t* temp = it;
            it = co_list_get_next_iterator(net_selector->io_ctx_trash, it);

            co_list_remove_at(net_selector->io_ctx_trash, temp);
        }
        else
        {
            it = co_list_get_next_iterator(net_selector->io_ctx_trash, it);
        }
    }
}

#endif // CO_OS_WIN
