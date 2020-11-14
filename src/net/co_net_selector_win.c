#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_tcp_client.h>
#include <coldforce/net/co_tcp_win.h>

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

    net_selector->items =
        co_array_create(sizeof(OVERLAPPED_ENTRY), NULL);
    co_array_set_size(net_selector->items, 1);

    return net_selector;
}

void
co_net_selector_destroy(
    co_net_selector_t* net_selector
)
{
    co_assert(co_array_get_size(net_selector->items) == 1);

    co_array_destroy(net_selector->items);

    CloseHandle(net_selector->iocp);

    co_mem_free(net_selector);
}

bool
co_net_selector_register(
    co_net_selector_t* net_selector,
    co_socket_t* sock
)
{
    co_socket_handle_set_blocking(sock->handle, false);

    if (CreateIoCompletionPort(
        (HANDLE)sock->handle, (HANDLE)net_selector->iocp, 0, 0) == NULL)
    {
        return false;
    }

    co_array_set_size(net_selector->items,
        co_array_get_size(net_selector->items) + 1);

    return true;
}

void
co_net_selector_unregister(
    co_net_selector_t* net_selector,
    co_socket_t* sock)
{
    size_t size = co_array_get_size(net_selector->items);
    co_assert(size > 1);

    CancelIo((HANDLE)sock->handle);

    co_socket_handle_set_blocking(sock->handle, true);

    co_array_set_size(net_selector->items, size - 1);
}

co_wait_result_t
co_net_selector_wait(
    co_net_selector_t* net_selector,
    uint32_t msec
)
{
    co_array_zero_clear(net_selector->items);

    LPOVERLAPPED_ENTRY entries =
        (LPOVERLAPPED_ENTRY)co_array_get(net_selector->items, 0);

    ULONG size = (ULONG)co_array_get_size(net_selector->items);
    ULONG removed = 0;

    if (GetQueuedCompletionStatusEx(
        net_selector->iocp, entries, size, &removed, msec, FALSE))
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
                size_t received_size =
                    (size_t)entries[index].dwNumberOfBytesTransferred;

                if (received_size > 0)
                {
                    if (!co_event_send(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_RECEIVE,
                        (uintptr_t)io_ctx->sock, (uintptr_t)received_size))
                    {
                        co_tcp_client_on_receive(
                            (co_tcp_client_t*)io_ctx->sock, received_size);
                    }
                }
                else
                {
                    if (!co_event_send(
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
                if (io_ctx->valid)
                {
                    co_event_send(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_SEND,
                        (uintptr_t)io_ctx->sock,
                        (uintptr_t)entries[index].dwNumberOfBytesTransferred);
                }

                break;
            }
            case CO_WIN_NET_IO_ID_TCP_ACCEPT:
            {
                if (io_ctx->valid)
                {
                    co_event_send(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_TCP_ACCEPT,
                        (uintptr_t)io_ctx->sock,
                        0);
                }

                break;
            }
            case CO_WIN_NET_IO_ID_TCP_CONNECT:
            {
                if (!io_ctx->valid)
                {
                    break;
                }

                uint32_t error_code = 0xFFFFFFFF;

                int seconds;
                size_t length = sizeof(seconds);

                if (co_socket_handle_get_option(
                    io_ctx->sock->handle,
                    SOL_SOCKET, SO_CONNECT_TIME, &seconds, &length))
                {
                    if (seconds != 0xFFFFFFFF)
                    {
                        error_code = 0;
                    }
                }

                co_event_send(
                    io_ctx->sock->owner_thread,
                    CO_NET_EVENT_ID_TCP_CONNECT,
                    (uintptr_t)io_ctx->sock,
                    error_code);

                break;
            }
            case CO_WIN_NET_IO_ID_UDP_SEND:
            {
                if (io_ctx->valid)
                {
                    co_event_send(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_UDP_SEND,
                        (uintptr_t)io_ctx->sock,
                        (uintptr_t)entries[index].dwNumberOfBytesTransferred);
                }

                break;
            }
            case CO_WIN_NET_IO_ID_UDP_RECEIVE:
            {
                if (io_ctx->valid)
                {
                    co_event_send(
                        io_ctx->sock->owner_thread,
                        CO_NET_EVENT_ID_UDP_RECEIVE,
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

void
co_win_free_net_io_ctx(
    co_win_net_io_ctx_t* io_ctx
)
{
    io_ctx->valid = false;
    co_mem_free_later(io_ctx);
}
