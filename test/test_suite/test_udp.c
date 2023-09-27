#include "test_udp.h"
#include "test_app.h"

#include <assert.h>

typedef struct
{
    test_udp_packet_header_st* data;
    size_t size;

} test_udp_data_block_st;

static void test_udp_client_stop(test_udp_thread_st* self, test_udp_client_st* test_udp_client)
{
    co_assert(
        test_udp_client->send_async_comp_count <=
        test_udp_client->send_async_count);

    if (test_udp_client->send_async_comp_count ==
        test_udp_client->send_async_count)
    {
        co_list_remove(self->test_udp_clients, test_udp_client);

        if (co_list_get_count(self->test_udp_clients) == 0)
        {
            co_thread_stop((co_thread_t*)self);
        }
    }
}

static void test_udp_client_destroy(test_udp_client_st* test_udp_client)
{
    test_udp_thread_st* self =
        (test_udp_thread_st*)co_thread_get_current();

    test_info("udp client: %d (%d-%d/%d)",
        co_list_get_count(self->test_udp_clients) - 1,
        test_udp_client->send_count,
        test_udp_client->send_async_comp_count,
        test_udp_client->send_async_count);

    co_udp_destroy(test_udp_client->udp_client);
    co_byte_array_destroy(test_udp_client->send_data);
    co_byte_array_destroy(test_udp_client->receive_data);
    co_timer_destroy(test_udp_client->send_timer);
    co_mem_free(test_udp_client);
}

static void test_udp_thread_on_send_async(test_udp_thread_st* self, co_udp_t* udp_client, void* user_data, bool result)
{
    (void)self;

    test_udp_client_st* test_udp_client =
        (test_udp_client_st*)co_udp_get_user_data(udp_client);
    co_assert(test_udp_client != NULL);

    co_assert(result);

    test_udp_data_block_st* data_block =
        (test_udp_data_block_st*)user_data;

    uint8_t* send_data =
        co_byte_array_get_ptr(
            test_udp_client->send_data, 0);
    size_t send_size =
        co_byte_array_get_count(test_udp_client->send_data);

    test_udp_packet_header_st* packet = data_block->data;

    packet->seq = co_byte_order_32_network_to_host(packet->seq);
    packet->size = co_byte_order_32_network_to_host(packet->size);
    packet->index = co_byte_order_32_network_to_host(packet->index);

    void* payload_data =
        ((uint8_t*)packet) + TEST_UDP_PACKET_HEADER_SIZE;

    if (packet->index >= send_size ||
        (memcmp(&send_data[packet->index], payload_data, packet->size) != 0))
    {
        test_error("Failed: test_udp_thread_on_send_async\n");
        exit(-1);
    }

    co_mem_free(packet);
    co_mem_free(data_block);

    test_udp_client->send_async_comp_count++;

    if (test_udp_client->total_sent_count > 0 &&
        test_udp_client->total_sent_count == test_udp_client->receive_count)
    {
        test_udp_client_stop(self, test_udp_client);
    }
}

static void test_udp_thread_on_receive(test_udp_thread_st* self, co_udp_t* udp_client)
{
    test_udp_client_st* test_udp_client =
        (test_udp_client_st*)co_udp_get_user_data(udp_client);
    co_assert(test_udp_client != NULL);

    for (;;)
    {
        char data[2048];

        co_net_addr_t remote_net_addr;
        ssize_t size = co_udp_receive(udp_client, &remote_net_addr, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }

        test_udp_packet_header_st* packet = (test_udp_packet_header_st*)data;

        packet->seq = co_byte_order_32_network_to_host(packet->seq);
        packet->size = co_byte_order_32_network_to_host(packet->size);
        packet->index = co_byte_order_32_network_to_host(packet->index);

        size_t send_count =
            test_udp_client->send_count + test_udp_client->send_async_count;
        size_t send_size =
            co_byte_array_get_count(test_udp_client->send_data);

        if ((packet->index + packet->size) > send_size)
        {
            test_error("Failed: test_udp_thread_on_receive 1");
            exit(-1);
        }

        if (packet->seq > send_count)
        {
            test_error("Failed: test_udp_thread_on_receive 2");
            exit(-1);
        }

        uint8_t* receive_data =
            co_byte_array_get_ptr(test_udp_client->receive_data, packet->index);
        memcpy(receive_data, ((uint8_t*)packet) + TEST_UDP_PACKET_HEADER_SIZE, packet->size);

        test_udp_client->receive_count++;

        if (test_udp_client->total_sent_count > 0 &&
            test_udp_client->total_sent_count == test_udp_client->receive_count)
        {
            if (memcmp(
                co_byte_array_get_const_ptr(test_udp_client->receive_data, 0),
                co_byte_array_get_const_ptr(test_udp_client->send_data, 0),
                send_size) != 0)
            {
                test_error("Failed: test_udp_client_on_receive 3");
                exit(-1);
            }

            test_udp_client_stop(self, test_udp_client);
        }
    }
}

static void test_udp_thread_on_timer(test_udp_thread_st* self, co_timer_t* timer)
{
    (void)self;

    test_udp_client_st* test_udp_client =
        (test_udp_client_st*)co_timer_get_user_data(timer);
    co_assert(test_udp_client != NULL);

    size_t total_size =
        co_byte_array_get_count(test_udp_client->send_data);
    size_t remaining =
        total_size - test_udp_client->send_index;
    uint32_t r = co_random_range(10, 1000);

    size_t payload_size = co_min(remaining, r);

    uint8_t* payload_data =
        co_byte_array_get_ptr(
            test_udp_client->send_data, test_udp_client->send_index);

    size_t packet_size = TEST_UDP_PACKET_HEADER_SIZE + payload_size;

    test_udp_packet_header_st* packet =
        (test_udp_packet_header_st*)co_mem_alloc(packet_size);
    memset(packet, 0x00, packet_size);

    size_t seq =
        test_udp_client->send_count +
        test_udp_client->send_async_count + 1;

    packet->seq = co_byte_order_32_host_to_network((uint32_t)seq);
    packet->size = co_byte_order_32_host_to_network((uint32_t)payload_size);
    packet->index = co_byte_order_32_host_to_network((uint32_t)test_udp_client->send_index);

    memcpy(((uint8_t*)packet) + TEST_UDP_PACKET_HEADER_SIZE, payload_data, payload_size);

    test_udp_client->send_index += payload_size;

    if (co_random_range(0, 1) == 0)
    {
        test_udp_data_block_st* data_block =
            (test_udp_data_block_st*)co_mem_alloc(sizeof(test_udp_data_block_st));

        data_block->data = packet;
        data_block->size = packet_size;

        if (!co_udp_send_async(
            test_udp_client->udp_client,
            &self->remote_net_addr, packet, packet_size, data_block))
        {
            test_error("Failed: test_udp_thread_on_timer(co_udp_send_async)");
            exit(-1);
        }

        test_udp_client->send_async_count++;
    }
    else
    {
        if (!co_udp_send(
            test_udp_client->udp_client,
            &self->remote_net_addr, packet, packet_size))
        {
            test_error("Failed: test_udp_thread_on_timer(co_udp_send)");
            exit(-1);
        }

        test_udp_client->send_count++;

        co_mem_free(packet);
    }

    if (test_udp_client->send_index != total_size)
    {
        co_timer_set_time(test_udp_client->send_timer, co_random_range(1, 200));
        co_timer_start(test_udp_client->send_timer);
    }
    else
    {
        test_udp_client->total_sent_count =
            test_udp_client->send_count + test_udp_client->send_async_count;
    }
}

static bool test_udp_thread_on_create(test_udp_thread_st* self)
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
        callbacks->on_send_async = (co_udp_send_async_fn)test_udp_thread_on_send_async;

        if (!co_udp_receive_start(udp_client))
        {
            test_error("Failed: test_udp_thread_on_create(co_udp_receive_start)");
            exit(-1);
        }

        test_udp_client_st* test_udp_client =
            (test_udp_client_st*)co_mem_alloc(sizeof(test_udp_client_st));

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
        test_udp_client->total_sent_count = 0;
        test_udp_client->receive_count = 0;

        co_list_add_tail(self->test_udp_clients, test_udp_client);

        co_byte_array_set_count(test_udp_client->send_data, self->data_size);
        co_random(
            co_byte_array_get_ptr(test_udp_client->send_data, 0),
            self->data_size);
        co_byte_array_set_count(test_udp_client->receive_data, self->data_size);

        co_udp_set_user_data(udp_client, test_udp_client);

        co_net_addr_set_address(&self->remote_net_addr, self->server_address);
        co_net_addr_set_port(&self->remote_net_addr, self->server_port);

        co_timer_start(test_udp_client->send_timer);
    }

    return true;
}

static void test_udp_thread_on_destroy(test_udp_thread_st* self)
{
    test_udp_server_thread_stop(&self->test_udp_server_thread);

    if (co_list_get_count(self->test_udp_clients) != 0)
    {
        test_error("Failed: test_udp_thread_on_destroy");

        co_thread_set_exit_code(-1);
    }

    co_list_destroy(self->test_udp_clients);

    co_thread_send_event(
        co_thread_get_parent((co_thread_t*)self),
        TEST_EVENT_ID_TEST_FINISHED, 0, 0);
}

void test_udp_run(test_udp_thread_st* test_udp_thread)
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
