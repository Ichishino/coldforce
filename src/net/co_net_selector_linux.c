#include <coldforce/core/co_std.h>

#include <coldforce/net/co_net_selector.h>
#include <coldforce/net/co_net_event.h>
#include <coldforce/net/co_socket_option.h>
#include <coldforce/net/co_tcp_client.h>

#ifdef CO_OS_LINUX

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>

//---------------------------------------------------------------------------//
// net selector (linux)
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

    int e_fd = epoll_create1(0);

    if (e_fd == -1)
    {
        co_mem_free(net_selector);

        return NULL;
    }

    int cancel_e_fd = eventfd(0, (EFD_NONBLOCK | EFD_SEMAPHORE));

    if (cancel_e_fd == -1)
    {
        close(e_fd);
        co_mem_free(net_selector);

        return NULL;
    }

    struct epoll_event e = { 0 };

    e.events = EPOLLIN;
    e.data.ptr = NULL;

    if (epoll_ctl(e_fd, EPOLL_CTL_ADD, cancel_e_fd, &e) != 0)
    {
        close(e_fd);
        close(cancel_e_fd);

        co_mem_free(net_selector);

        return NULL;
    }

    net_selector->e_fd = e_fd;
    net_selector->cancel_e_fd = cancel_e_fd;
    net_selector->sock_count = 0;

    net_selector->e_entries =
        co_array_create(sizeof(struct epoll_event));

    return net_selector;
}

void
co_net_selector_destroy(
    co_net_selector_t* net_selector
)
{
    co_assert(net_selector->sock_count == 0);

    if ((net_selector->e_fd >= 0) && (net_selector->cancel_e_fd >= 0))
    {
        struct epoll_event e;
        epoll_ctl(net_selector->e_fd, EPOLL_CTL_DEL, net_selector->cancel_e_fd, &e);
    }

    if (net_selector->cancel_e_fd >= 0)
    {
        close(net_selector->cancel_e_fd);
    }

    if (net_selector->e_fd >= 0)
    {
        close(net_selector->e_fd);
    }

    co_array_destroy(net_selector->e_entries);

    co_mem_free(net_selector);
}

bool
co_net_selector_register(
    co_net_selector_t* net_selector,
    co_socket_t* sock,
    uint32_t flags
)
{
    struct epoll_event e = { 0 };

    e.events = flags | EPOLLET;
    e.data.ptr = sock;

    if (epoll_ctl(net_selector->e_fd, EPOLL_CTL_ADD, sock->handle, &e) != 0)
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
    co_socket_t* sock)
{
    co_assert(net_selector->sock_count > 0);

    struct epoll_event e;

    epoll_ctl(net_selector->e_fd, EPOLL_CTL_DEL, sock->handle, &e);

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
    co_assert(net_selector->sock_count > 0);

    struct epoll_event e;

    e.events = flags | EPOLLET;
    e.data.ptr = sock;

    if (epoll_ctl(net_selector->e_fd, EPOLL_CTL_MOD, sock->handle, &e) != 0)
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
    co_wait_result_t result = CO_WAIT_RESULT_SUCCESS;

    co_array_set_count(net_selector->e_entries, net_selector->sock_count + 1);
    co_array_zero_clear(net_selector->e_entries);

    struct epoll_event* events =
        (struct epoll_event*)co_array_get_ptr(net_selector->e_entries, 0);

    int e_count = (int)co_array_get_count(net_selector->e_entries);

    int count = epoll_wait(net_selector->e_fd, events, e_count, (int)msec);

    if (count > 0)
    {
        for (int index = 0; index < count; ++index)
        {
            struct epoll_event* e = &events[index];

            if (e->data.ptr == NULL)
            {
                eventfd_t val;
                eventfd_read(net_selector->cancel_e_fd, &val);
            }
            else
            {
                co_socket_t* sock = (co_socket_t*)e->data.ptr;

                int error_code = 0;

                if (e->events & EPOLLERR)
                {
                    co_socket_option_get_error(sock, &error_code);
                }

                if (e->events & EPOLLHUP)
                {
                    if (sock->type == CO_SOCKET_TYPE_TCP_CONNECTION)
                    {
                        if (!co_thread_send_event(sock->owner_thread,
                            CO_NET_EVENT_ID_TCP_CLOSE, (uintptr_t)sock, error_code))
                        {
                            co_tcp_client_on_close((co_tcp_client_t*)sock);
                        }

                        continue;
                    }
                }

                if (e->events & EPOLLIN)
                {
                    co_thread_send_event(
                        sock->owner_thread,
                        net_event_ids[sock->type - 1].read,
                        (uintptr_t)sock,
                        error_code);
                }

                if (e->events & EPOLLOUT)
                {
                    co_thread_send_event(
                        sock->owner_thread,
                        net_event_ids[sock->type - 1].write,
                        (uintptr_t)sock,
                        error_code);
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
    eventfd_write(net_selector->cancel_e_fd, 1);
}

#endif // CO_OS_LINUX
