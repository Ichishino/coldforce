#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_socket_option.h>
#include <coldforce/net/co_tcp_client.h>

#ifdef CO_OS_MAC

#include <sys/event.h>
#include <sys/time.h>
#include <sys/errno.h>

//---------------------------------------------------------------------------//
// net selector (mac)
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

struct co_net_event_ids
{
    co_event_id_t write;
    co_event_id_t read;
};

static const struct co_net_event_ids net_event_ids[4] =
{
    { 0, CO_NET_EVENT_ID_TCP_ACCEPT_READY },

    { CO_NET_EVENT_ID_TCP_CONNECT_COMPLETE, 0 },

    { CO_NET_EVENT_ID_TCP_SEND_READY,
      CO_NET_EVENT_ID_TCP_RECEIVE_READY },

    { CO_NET_EVENT_ID_UDP_SEND_READY,
      CO_NET_EVENT_ID_UDP_RECEIVE_READY }
};

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

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

    if (pipe(net_selector->cancel_fds) != 0)
    {
        co_mem_free(net_selector);

        return NULL;
    }

    net_selector->kqueue_fd = kqueue();

    if (net_selector->kqueue_fd == -1)
    {
        close(net_selector->cancel_fds[0]);
        close(net_selector->cancel_fds[1]);
        co_mem_free(net_selector);

        return NULL;
    }

    net_selector->sock_count = 0;
    net_selector->entries =
        co_array_create(sizeof(struct kevent));

    co_socket_t sock = { 0 };
    sock.handle = net_selector->cancel_fds[0];

    co_net_selector_register(
        net_selector, &sock, CO_SOCKET_EVENT_CANCEL);

    return net_selector;
}

void
co_net_selector_destroy(
    co_net_selector_t* net_selector
)
{
    if (net_selector != NULL)
    {
        co_socket_t sock = { 0 };
        sock.handle = net_selector->cancel_fds[0];

        co_net_selector_unregister(net_selector, &sock);

        co_assert(net_selector->sock_count == 0);

        close(net_selector->cancel_fds[0]);
        close(net_selector->cancel_fds[1]);
        close(net_selector->kqueue_fd);

        co_array_destroy(net_selector->entries);

        co_mem_free(net_selector);
    }
}

bool
co_net_selector_register(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags
)
{
    int ev_count = 0;
    struct kevent ev[2] = { 0 };

    if (flags & CO_SOCKET_EVENT_RECEIVE)
    {
        EV_SET(&ev[0], sock->handle,
            EVFILT_READ, (EV_ADD | EV_CLEAR), 0, 0, sock);
        ++ev_count;
    }

    if (flags & CO_SOCKET_EVENT_SEND)
    {
        EV_SET(&ev[ev_count], sock->handle,
            EVFILT_WRITE, (EV_ADD | EV_CLEAR), 0, 0, sock);
        ++ev_count;
    }

    if (ev_count == 0)
    {
        if (flags == CO_SOCKET_EVENT_ACCEPT)
        {
            EV_SET(&ev[0], sock->handle,
                EVFILT_READ, (EV_ADD | EV_CLEAR), 0, 0, sock);
            ev_count = 1;
        }
        else if (flags == CO_SOCKET_EVENT_CONNECT)
        {
            EV_SET(&ev[0], sock->handle,
                EVFILT_WRITE, (EV_ADD | EV_CLEAR), 0, 0, sock);
            ev_count = 1;
        }
        else if (flags == CO_SOCKET_EVENT_CANCEL)
        {
            EV_SET(&ev[0], sock->handle,
                EVFILT_READ, (EV_ADD | EV_CLEAR), 0, 0, NULL);
            ev_count = 1;
        }
        else
        {
            return false;
        }
    }

    if (kevent(net_selector->kqueue_fd,
        ev, ev_count, NULL, 0, NULL) == -1)
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

    struct kevent ev[2] = { 0 };

    EV_SET(&ev[0], sock->handle, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    EV_SET(&ev[1], sock->handle, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);

    kevent(net_selector->kqueue_fd, ev, 2, NULL, 0, NULL);

    co_socket_handle_set_blocking(sock->handle, true);

    --net_selector->sock_count;
}

bool
co_net_selector_update(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags
)
{
    co_assert(net_selector->sock_count > 1);

    struct kevent ev[2];

    if (flags & CO_SOCKET_EVENT_RECEIVE)
    {
        EV_SET(&ev[0], sock->handle,
            EVFILT_READ, (EV_ADD | EV_CLEAR), 0, 0, sock);
    }
    else
    {
        EV_SET(&ev[0], sock->handle,
            EVFILT_READ, EV_DISABLE, 0, 0, sock);
    }

    if (flags & CO_SOCKET_EVENT_SEND)
    {
        EV_SET(&ev[1], sock->handle,
            EVFILT_WRITE, (EV_ADD | EV_CLEAR), 0, 0, sock);
    }
    else
    {
        EV_SET(&ev[1], sock->handle,
            EVFILT_WRITE, EV_DISABLE, 0, 0, sock);
    }

    if (kevent(net_selector->kqueue_fd, ev, 2, NULL, 0, NULL) == -1)
    {
        return false;
    }
    
    return true;
}

co_wait_result_t
co_net_selector_wait(
    co_net_selector_t* net_selector,
    uint32_t msec
)
{
    co_assert(net_selector->sock_count > 0);

    co_wait_result_t result = CO_WAIT_RESULT_SUCCESS;

    struct timespec ts = { 0 };

    if (msec != CO_INFINITE)
    {
        ts.tv_sec = msec / 1000;
        ts.tv_nsec = msec % 1000 * 1000000;
    }

    co_array_set_count(net_selector->entries, net_selector->sock_count);
    co_array_zero_clear(net_selector->entries);

    struct kevent* events =
        (struct kevent*)co_array_get_ptr(net_selector->entries, 0);
    int event_count = (int)co_array_get_count(net_selector->entries);

    int count = kevent(net_selector->kqueue_fd,
        0, 0, events, event_count, ((msec == CO_INFINITE) ? NULL : &ts));

    if (count > 0)
    {
        for (int index = 0; index < count; ++index)
        {
            if (events[index].udata == 0)
            {
                char value[32];
                while (read(net_selector->cancel_fds[0],
                    value, sizeof(value)) > 0);
            }
            else
            {
                co_socket_t* sock = (co_socket_t*)events[index].udata;

                int error_code = 0;

                if (events[index].flags & EV_ERROR)
                {
                    error_code = (int)events[index].data;
                }

                if (events[index].flags & EV_EOF)
                {
                    if (error_code == 0)
                    {
                        error_code = events[index].fflags;
                    }

                    if (error_code == 0)
                    {
                        co_socket_option_get_error(sock, &error_code);
                    }
                }

                if (events[index].filter == EVFILT_READ)
                {
                    if ((events[index].data > 0) ||
                        (sock->type == CO_SOCKET_TYPE_TCP_SERVER))
                    {
                        co_event_send(sock->owner_thread,
                            net_event_ids[sock->type - 1].read,
                            (uintptr_t)sock, 0);
                    }
                    else
                    {
                        if (!co_event_send(sock->owner_thread,
                            CO_NET_EVENT_ID_TCP_CLOSE,
                            (uintptr_t)sock, 0))
                        {
                            co_tcp_client_on_close((co_tcp_client_t*)sock);
                        }
                    }
                }
                else if (events[index].filter == EVFILT_WRITE)
                {
                    co_event_send(sock->owner_thread,
                        net_event_ids[sock->type - 1].write,
                        (uintptr_t)sock, error_code);
                }
            }
        }
    }
    else if (count == 0)
    {
        result = CO_WAIT_RESULT_TIMEOUT;
    }
    else if (co_socket_get_error() != EINTR)
    {
        result = CO_WAIT_RESULT_ERROR;
    }

    return result;
}

void
co_net_selector_wake_up(
    co_net_selector_t* net_selector
)
{
    char value = 1;
    write(net_selector->cancel_fds[1], &value, sizeof(value));
}

#endif // CO_OS_MAC
