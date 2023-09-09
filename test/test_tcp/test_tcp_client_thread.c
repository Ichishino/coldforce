#include "test_tcp_client_thread.h"

static co_tcp_client_t*
try_connect(
    tcp_client_thread* self,
    size_t id
);

static void
on_tcp_receive(
    tcp_client_thread* self,
    co_tcp_client_t* client
)
{
    for (;;)
    {
        char buffer[1024];

        ssize_t size =
            co_tcp_receive(client, buffer, sizeof(buffer));

        if (size <= 0)
        {
            return;
        }

        co_assert(size <= TCP_CLIENT_DATA_SIZE);

        size_t id = (size_t)co_tcp_get_user_data(client);
        co_assert(id >= 1 && id <= self->client_count);
        test_client* tc = &self->clients[id - 1];

        if (memcmp(&tc->send_data[tc->data_index], buffer, size) != 0)
        {
            co_assert(false);
        }

        tc->data_index += size;
        co_assert(tc->data_index <= TCP_CLIENT_DATA_SIZE);

        if (tc->data_index == TCP_CLIENT_DATA_SIZE)
        {
            co_timer_set_time(tc->send_timer, co_random_range(100, 1000));
            co_timer_start(tc->send_timer);
        }
    }
}

static void
on_tcp_close(
    tcp_client_thread* self,
    co_tcp_client_t* client
)
{
    co_log_info(
        LOG_CATEGORY_TEST_TCP_CLIENT,
        "connect closed");

    size_t id = (size_t)co_tcp_get_user_data(client);
    co_assert(id >= 1 && id <= self->client_count);

    test_client* tc = &self->clients[id - 1];

    co_tcp_client_destroy(tc->client);
    tc->client = NULL;

    co_timer_stop(tc->send_timer);

    co_timer_set_time(tc->retry_connect_timer, co_random_range(1000, 10000));
    co_timer_start(tc->retry_connect_timer);
}

static void
on_send_timer(
    tcp_client_thread* self,
    co_timer_t* timer
)
{
    size_t id = (size_t)co_timer_get_user_data(timer);
    co_assert(id >= 1 && id <= self->client_count);
    co_assert(self->clients[id - 1].client != NULL);
    test_client* tc = &self->clients[id - 1];

    tc->data_index = 0;
    co_random(tc->send_data, TCP_CLIENT_DATA_SIZE);

    co_tcp_send(tc->client, tc->send_data, TCP_CLIENT_DATA_SIZE);
}

static void
on_retry_connect_timer(
    tcp_client_thread* self,
    co_timer_t* timer
)
{
    size_t id = (size_t)co_timer_get_user_data(timer);
    co_assert(id >= 1 && id <= self->client_count);
    co_assert(self->clients[id - 1].client == NULL);
    test_client* tc = &self->clients[id - 1];

    tc->client = try_connect(self, id);
}

static void
on_tcp_connect(
    tcp_client_thread* self,
    co_tcp_client_t* client,
    int error_code
)
{
    size_t id = (size_t)co_tcp_get_user_data(client);
    co_assert(id >= 1 && id <= self->client_count);
    test_client* tc = &self->clients[id - 1];

    co_log_info(
        LOG_CATEGORY_TEST_TCP_CLIENT,
        "connect result: %d",
        error_code);

    if (error_code == 0)
    {
        co_timer_set_time(tc->send_timer, co_random_range(100, 1000));
        co_timer_start(tc->send_timer);
    }
    else
    {
        co_tcp_client_destroy(tc->client);
        tc->client = NULL;

        co_timer_set_time(tc->retry_connect_timer, co_random_range(1000, 10000));
        co_timer_start(tc->retry_connect_timer);
    }
}

static bool
on_tcp_client_thread_create(
    tcp_client_thread* self
)
{
    self->clients = (test_client*)co_mem_alloc(
        sizeof(test_client) * self->client_count);

    for (size_t index = 0; index < self->client_count; ++index)
    {
        size_t id = index + 1;

        self->clients[index].client = try_connect(self, id);
        self->clients[index].retry_connect_timer =
            co_timer_create(CO_INFINITE,
                (co_timer_fn)on_retry_connect_timer, false, (void*)id);
        self->clients[index].send_timer =
            co_timer_create(CO_INFINITE,
                (co_timer_fn)on_send_timer, false, (void*)id);
    }

    return true;
}

static void
on_tcp_client_thread_destroy(
    tcp_client_thread* self
)
{
    for (size_t index = 0; index < self->client_count; ++index)
    {
        co_tcp_client_destroy(self->clients[index].client);
        co_timer_destroy(self->clients[index].retry_connect_timer);
        co_timer_destroy(self->clients[index].send_timer);
    }

    co_mem_free(self->clients);
}

static co_tcp_client_t*
try_connect(
    tcp_client_thread* self,
    size_t id
)
{
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

    co_tcp_client_t* client = co_tcp_client_create(&local_net_addr);
    co_assert(client != NULL);

    if (client == NULL)
    {
        return NULL;
    }

    co_net_addr_t remote_net_addr = { 0 };
    co_net_addr_set_address(&remote_net_addr, self->server_address);
    co_net_addr_set_port(&remote_net_addr, self->server_port);

    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(client);
    callbacks->on_connect = (co_tcp_connect_fn)on_tcp_connect;
    callbacks->on_receive = (co_tcp_receive_fn)on_tcp_receive;
    callbacks->on_close = (co_tcp_close_fn)on_tcp_close;

    bool result = co_tcp_connect(client, &remote_net_addr);
    co_assert(result);

    if (!result)
    {
        co_tcp_client_destroy(client);

        return NULL;
    }

    co_tcp_set_user_data(client, (void*)id);

    return client;
}

bool
tcp_client_thread_start(
    tcp_client_thread* thread
)
{
    co_net_thread_setup(
        (co_thread_t*)thread, "tcp_client_thread",
        (co_thread_create_fn)on_tcp_client_thread_create,
        (co_thread_destroy_fn)on_tcp_client_thread_destroy);

    return co_thread_start((co_thread_t*)thread);
}
