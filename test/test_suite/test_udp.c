#include "test_udp.h"
#include "test_app.h"

#include <assert.h>

static void test_udp_thread_on_send_async(test_udp_thread_t* self, co_udp_t* udp_client, bool result)
{
    (void)self;

    test_udp_client_t* test_udp_client =
        (test_udp_client_t*)co_udp_get_user_data(udp_client);
    assert(test_udp_client != NULL);

    test_udp_client->send_async_comp_count++;

    co_assert(result);
}

static void test_udp_thread_on_receive(test_udp_thread_t* self, co_udp_t* udp_client)
{
    test_udp_client_t* test_udp_client =
        (test_udp_client_t*)co_udp_get_user_data(udp_client);
    assert(test_udp_client != NULL);

    for (;;)
    {
        char data[1024];

        co_net_addr_t remote_net_addr;
        ssize_t size = co_udp_receive(udp_client, &remote_net_addr, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }

        co_byte_array_add(test_udp_client->receive_data, data, size);

        size_t test_size =
            co_byte_array_get_count(test_udp_client->receive_data);

        if (test_size >=
            co_byte_array_get_count(test_udp_client->send_data))
        {
            if (memcmp(
                co_byte_array_get_const_ptr(test_udp_client->receive_data, 0),
                co_byte_array_get_const_ptr(test_udp_client->send_data, 0),
                test_size) != 0)
            {
                test_error("Failed: test_udp_client_on_receive\n");
                exit(-1);
            }

            co_list_remove(self->test_udp_clients, test_udp_client);

            if (co_list_get_count(self->test_udp_clients) == 0)
            {
                co_thread_stop((co_thread_t*)self);
            }
        }
    }
}

static void test_udp_thread_on_timer(test_udp_thread_t* self, co_timer_t* timer)
{
    (void)self;

    test_udp_client_t* test_udp_client =
        (test_udp_client_t*)co_timer_get_user_data(timer);

    size_t total_size =
        co_byte_array_get_count(test_udp_client->send_data);
    size_t remaining =
        total_size - test_udp_client->send_index;
    uint32_t r = co_random_range(10, 1000);
    size_t size = co_min(remaining, r);

    const void* data =
        co_byte_array_get_ptr(
            test_udp_client->send_data, test_udp_client->send_index);
    test_udp_client->send_index += size;

    if (co_random_range(0, 1) == 0)
    {
        if (!co_udp_send_async(test_udp_client->udp_client, &self->remote_net_addr, data, size))
        {
            test_error("Failed: test_udp_thread_on_timer(co_udp_send_async)");
            exit(-1);
        }

        test_udp_client->send_async_count++;
    }
    else
    {
        if (!co_udp_send(test_udp_client->udp_client, &self->remote_net_addr, data, size))
        {
            test_error("Failed: test_udp_thread_on_timer(co_udp_send)");
            exit(-1);
        }

        test_udp_client->send_count++;
    }

    if (test_udp_client->send_index != total_size)
    {
        co_timer_set_time(test_udp_client->send_timer, co_random_range(1, 200));
        co_timer_start(test_udp_client->send_timer);
    }
}

static void test_udp_client_destroy(test_udp_client_t* test_udp_client)
{
    test_udp_thread_t* self =
        (test_udp_thread_t*)co_thread_get_current();

    test_info("udp client: %d (%d-%d/%d)",
        co_list_get_count(self->test_udp_clients) - 1,
        test_udp_client->send_count,
        test_udp_client->send_async_comp_count,
        test_udp_client->send_async_count);

    co_udp_destroy(test_udp_client->udp_client);
    co_byte_array_destroy(test_udp_client->send_data);
    co_byte_array_destroy(test_udp_client->receive_data);
    co_timer_destroy(test_udp_client->send_timer);
    free(test_udp_client);
}

static bool test_udp_thread_on_create(test_udp_thread_t* self)
{
    // server

    self->test_udp_server_thread.family = self->family;
    self->test_udp_server_thread.port = self->server_port;
    test_udp_server_thread_start(&self->test_udp_server_thread);

    // clients

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)test_udp_client_destroy;
    self->test_udp_clients = co_list_create(&list_ctx);

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, self->family);

    for (size_t n = 0; n < self->client_count; n++)
    {
        co_udp_t* udp_client = co_udp_create(&local_net_addr);

        if (udp_client == NULL)
        {
            test_error("Failed: co_udp_create");
            exit(-1);
        }

        co_udp_callbacks_st* callbacks = co_udp_get_callbacks(udp_client);
        callbacks->on_receive = (co_udp_receive_fn)test_udp_thread_on_receive;
        callbacks->on_send_async = (co_udp_send_fn)test_udp_thread_on_send_async;

        if (!co_udp_receive_start(udp_client))
        {
            test_error("Failed: test_udp_thread_on_create(co_udp_receive_start)");
            exit(-1);
        }

        test_udp_client_t* test_udp_client =
            (test_udp_client_t*)malloc(sizeof(test_udp_client_t));

        test_udp_client->udp_client = udp_client;
        test_udp_client->send_data = co_byte_array_create();
        test_udp_client->receive_data = co_byte_array_create();
        test_udp_client->send_timer =
            co_timer_create(co_random_range(1, 200),
                (co_timer_fn)test_udp_thread_on_timer, false, test_udp_client);
        test_udp_client->send_index = 0;
        test_udp_client->send_count = 0;
        test_udp_client->send_async_count = 0;
        test_udp_client->send_async_comp_count = 0;

        co_list_add_tail(self->test_udp_clients, test_udp_client);

        co_byte_array_set_count(test_udp_client->send_data, self->data_size);
        co_random(
            co_byte_array_get_ptr(test_udp_client->send_data, 0),
            self->data_size);

        co_udp_set_user_data(udp_client, test_udp_client);

        co_net_addr_set_address(&self->remote_net_addr, self->server_address);
        co_net_addr_set_port(&self->remote_net_addr, self->server_port);

        co_timer_start(test_udp_client->send_timer);
    }

    return true;
}

static void test_udp_thread_on_destroy(test_udp_thread_t* self)
{
    test_udp_server_thread_stop(&self->test_udp_server_thread);

    if (co_list_get_count(self->test_udp_clients) != 0)
    {
        test_error("Failed: test_udp_thread_on_destroy");
        exit(-1);
    }

    co_list_destroy(self->test_udp_clients);
}

void test_udp_run(test_udp_thread_t* test_udp_thread)
{
    co_net_thread_setup(
        (co_thread_t*)test_udp_thread, "test_udp_thread",
        (co_thread_create_fn)test_udp_thread_on_create,
        (co_thread_destroy_fn)test_udp_thread_on_destroy);

    if (!co_thread_start((co_thread_t*)test_udp_thread))
    {
        test_error("Failed: co_thread_start(test_udp_run)");
        exit(-1);
    }
}
