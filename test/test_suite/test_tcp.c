#include "test_tcp.h"
#include "test_app.h"

#include <assert.h>

typedef struct
{
    uint8_t* data;
    size_t size;

} test_tcp_data_block_st;

static void test_tcp_client_stop(test_tcp_thread_st* self, test_tcp_client_st* test_tcp_client)
{
    co_assert(
        test_tcp_client->send_async_comp_count <=
        test_tcp_client->send_async_count);

    if (test_tcp_client->send_async_comp_count ==
        test_tcp_client->send_async_count)
    {
        co_list_remove(self->test_tcp_clients, test_tcp_client);

        if (co_list_get_count(self->test_tcp_clients) == 0)
        {
            co_thread_send_event(
                (co_thread_t*)&self->test_tcp_server_thread,
                TEST_EVENT_TCP_SERVER_REQ_CLOSE, 0, 0);

            test_info("tcp thread send req-close");
        }
    }
}

static void test_tcp_client_destroy(test_tcp_client_st* test_tcp_client)
{
    test_tcp_thread_st* self =
        (test_tcp_thread_st*)co_thread_get_current();

    test_info("tcp client: %d (%d-%d/%d)",
        co_list_get_count(self->test_tcp_clients) - 1,
        test_tcp_client->send_count,
        test_tcp_client->send_async_comp_count,
        test_tcp_client->send_async_count);

    co_tcp_client_destroy(test_tcp_client->tcp_client);
    co_byte_array_destroy(test_tcp_client->send_data);
    co_byte_array_destroy(test_tcp_client->receive_data);
    co_timer_destroy(test_tcp_client->send_timer);
    co_mem_free(test_tcp_client);
}

static void test_tcp_thread_on_send_async(test_tcp_thread_st* self, co_tcp_client_t* tcp_client, void* user_data, bool result)
{
    (void)self;

    test_tcp_client_st* test_tcp_client =
        (test_tcp_client_st*)co_tcp_get_user_data(tcp_client);
    assert(test_tcp_client != NULL);

    co_assert(result);

    test_tcp_data_block_st* data_block =
        (test_tcp_data_block_st*)user_data;

    uint8_t* data =
        co_byte_array_get_ptr(
            test_tcp_client->send_data, 0);

    if ((data_block->data < data) ||
        ((data_block->data + data_block->size) >
            (data + co_byte_array_get_count(test_tcp_client->send_data))))
    {
        test_error("Failed: test_tcp_thread_on_send_async\n");
        exit(-1);
    }

    co_mem_free(data_block);

    test_tcp_client->send_async_comp_count++;

    if (co_byte_array_get_count(test_tcp_client->receive_data) ==
        co_byte_array_get_count(test_tcp_client->send_data))
    {
        test_tcp_client_stop(self, test_tcp_client);
    }
}

static void test_tcp_thread_on_receive(test_tcp_thread_st* self, co_tcp_client_t* tcp_client)
{
    test_tcp_client_st* test_tcp_client =
        (test_tcp_client_st*)co_tcp_get_user_data(tcp_client);
    assert(test_tcp_client != NULL);

    for (;;)
    {
        char data[1024];

        ssize_t size = co_tcp_receive(tcp_client, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }

        co_byte_array_add(test_tcp_client->receive_data, data, size);

        size_t receive_size =
            co_byte_array_get_count(test_tcp_client->receive_data);
        size_t send_size =
            co_byte_array_get_count(test_tcp_client->send_data);

        co_assert(send_size >= receive_size);

        if (receive_size == send_size)
        {
            if (memcmp(
                co_byte_array_get_const_ptr(test_tcp_client->receive_data, 0),
                co_byte_array_get_const_ptr(test_tcp_client->send_data, 0),
                receive_size) != 0)
            {
                test_error("Failed: test_tcp_client_on_receive");
                exit(-1);
            }

            if (co_random_range(0, 1) == 0)
            {
                test_tcp_client_stop(self, test_tcp_client);
            }
        }
    }
}

static void test_tcp_thread_on_close(test_tcp_thread_st* self, co_tcp_client_t* tcp_client)
{
    test_tcp_client_st* test_tcp_client =
        (test_tcp_client_st*)co_tcp_get_user_data(tcp_client);
    assert(test_tcp_client != NULL);

    test_tcp_client_stop(self, test_tcp_client);
}

static void test_tcp_thread_on_connect(test_tcp_thread_st* self, co_tcp_client_t* tcp_client, int error_code)
{
    (void)self;

    test_tcp_client_st* test_tcp_client =
        (test_tcp_client_st*)co_tcp_get_user_data(tcp_client);
    assert(test_tcp_client != NULL);

    if (error_code == 0)
    {
        co_timer_start(test_tcp_client->send_timer);
    }
    else
    {
        test_error("Failed: test_tcp_thread_on_connect");
        exit(-1);
    }
}

void test_tcp_thread_on_timer(test_tcp_thread_st* self, co_timer_t* timer)
{
    (void)self;

    test_tcp_client_st* test_tcp_client =
        (test_tcp_client_st*)co_timer_get_user_data(timer);

    size_t total_size =
        co_byte_array_get_count(test_tcp_client->send_data);
    size_t remaining =
        total_size - test_tcp_client->send_index;

    if (remaining > 0)
    {
        uint32_t r = co_random_range(10, 1000);
        size_t size = co_min(remaining, r);

        void* data =
            co_byte_array_get_ptr(
                test_tcp_client->send_data, test_tcp_client->send_index);
        test_tcp_client->send_index += size;

        if (co_random_range(0, 1) == 0)
        {
            test_tcp_data_block_st* data_block =
                (test_tcp_data_block_st*)co_mem_alloc(sizeof(test_tcp_data_block_st));

            data_block->data = data;
            data_block->size = size;

            if (!co_tcp_send_async(test_tcp_client->tcp_client, data, size, data_block))
            {
                test_error("Failed: test_tcp_thread_on_timer(co_tcp_send_async)");
                exit(-1);
            }

            test_tcp_client->send_async_count++;
        }
        else
        {
            if (!co_tcp_send(test_tcp_client->tcp_client, data, size))
            {
                test_error("Failed: test_tcp_thread_on_timer(co_tcp_send)");
                exit(-1);
            }

            test_tcp_client->send_count++;
        }
    }
    else
    {
        if (!co_tcp_send(test_tcp_client->tcp_client, "close", 5))
        {
            test_error("Failed: test_tcp_thread_on_timer(co_tcp_send)");
            exit(-1);
        }

        return;
    }

    if (test_tcp_client->send_index != total_size)
    {
        co_timer_set_time(test_tcp_client->send_timer, co_random_range(1, 200));
    }
    else
    {
        co_timer_set_time(test_tcp_client->send_timer, 1000);
    }

    co_timer_start(test_tcp_client->send_timer);
}

static void test_tcp_thread_on_res_close(test_tcp_thread_st* self, const co_event_st* event)
{
    (void)event;

    test_info("tcp thread receive res-close");

    co_thread_stop((co_thread_t*)self);
}

static bool test_tcp_thread_on_create(test_tcp_thread_st* self)
{
    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_TCP_SERVER_RES_CLOSE,
        (co_event_fn)test_tcp_thread_on_res_close);

    // server

    self->test_tcp_server_thread.family = self->family;
    self->test_tcp_server_thread.port = self->server_port;
    test_tcp_server_thread_start(&self->test_tcp_server_thread);

    // clients

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)test_tcp_client_destroy;
    self->test_tcp_clients = co_list_create(&list_ctx);

    for (size_t n = 0; n < self->client_count; n++)
    {
        co_net_addr_t local_net_addr = { 0 };
        co_net_addr_set_family(&local_net_addr, self->family);

        co_tcp_client_t* tcp_client = co_tcp_client_create(&local_net_addr);

        if (tcp_client == NULL)
        {
            test_error("Failed: co_tcp_client_create");
            exit(-1);
        }

        co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(tcp_client);
        callbacks->on_connect = (co_tcp_connect_fn)test_tcp_thread_on_connect;
        callbacks->on_close = (co_tcp_close_fn)test_tcp_thread_on_close;
        callbacks->on_receive = (co_tcp_receive_fn)test_tcp_thread_on_receive;
        callbacks->on_send_async = (co_tcp_send_async_fn)test_tcp_thread_on_send_async;

        test_tcp_client_st* test_tcp_client =
            (test_tcp_client_st*)co_mem_alloc(sizeof(test_tcp_client_st));

        test_tcp_client->tcp_client = tcp_client;
        test_tcp_client->send_data = co_byte_array_create();
        test_tcp_client->receive_data = co_byte_array_create();
        test_tcp_client->send_timer =
            co_timer_create(co_random_range(1, 200),
                (co_timer_fn)test_tcp_thread_on_timer, false, test_tcp_client);
        test_tcp_client->send_index = 0;
        test_tcp_client->send_count = 0;
        test_tcp_client->send_async_count = 0;
        test_tcp_client->send_async_comp_count = 0;

        co_list_add_tail(self->test_tcp_clients, test_tcp_client);

        co_byte_array_set_count(test_tcp_client->send_data, self->data_size);
        co_random(
            co_byte_array_get_ptr(test_tcp_client->send_data, 0),
            self->data_size);

        co_tcp_set_user_data(tcp_client, test_tcp_client);

        co_net_addr_t remote_net_addr = { 0 };
        co_net_addr_set_address(&remote_net_addr, self->server_address);
        co_net_addr_set_port(&remote_net_addr, self->server_port);

        if (!co_tcp_connect(tcp_client, &remote_net_addr))
        {
            test_error("Failed: co_tcp_connect");
            exit(-1);
        }
    }

    return true;
}

static void test_tcp_thread_on_destroy(test_tcp_thread_st* self)
{
    test_tcp_server_thread_stop(&self->test_tcp_server_thread);

    if (co_list_get_count(self->test_tcp_clients) != 0)
    {
        test_error("Failed: test_tcp_thread_on_destroy");

        co_thread_set_exit_code(-1);
    }

    co_list_destroy(self->test_tcp_clients);

    co_thread_send_event(
        co_thread_get_parent((co_thread_t*)self),
        TEST_EVENT_ID_TEST_FINISHED, 0, 0);
}

void test_tcp_run(test_tcp_thread_st* test_tcp_thread)
{
    co_net_thread_setup(
        (co_thread_t*)test_tcp_thread, "test_tcp_thread",
        (co_thread_create_fn)test_tcp_thread_on_create,
        (co_thread_destroy_fn)test_tcp_thread_on_destroy);

    if (!co_thread_start((co_thread_t*)test_tcp_thread))
    {
        test_error("Failed: co_thread_start(test_tcp_run)\n");
        exit(-1);
    }
}
