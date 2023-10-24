#include "test_tcp_server.h"
#include "test_app.h"

static void test_tcp_server_on_receive(test_tcp_server_thread_st* self, co_tcp_client_t* tcp_client)
{
    for (;;)
    {
        char data[1024];

        ssize_t size = co_tcp_receive(tcp_client, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }

        co_queue_t* receive_buffer =
            (co_queue_t*)co_tcp_get_user_data(tcp_client);

        co_queue_push_array(receive_buffer, data, size);

        for (;;)
        {
            if (co_queue_get_count(receive_buffer) >= TEST_TCP_PACKET_HEADER_SIZE)
            {
                test_tcp_packet_header_st header;
                co_queue_peek_array(receive_buffer, &header, TEST_TCP_PACKET_HEADER_SIZE);

                header.id =
                    co_byte_order_32_network_to_host(header.id);
                header.size =
                    co_byte_order_32_network_to_host(header.size);

                size_t packet_size =
                    header.size + TEST_TCP_PACKET_HEADER_SIZE;

                if (co_queue_get_count(receive_buffer) >= packet_size)
                {
                    if (header.id == 1)
                    {
                        char send_data[2048];
                        co_queue_pop_array(receive_buffer, send_data, packet_size);

                        co_tcp_send(
                            tcp_client,
                            &send_data[TEST_TCP_PACKET_HEADER_SIZE], header.size);
                    }
                    else if (header.id == 2)
                    {
                        co_queue_destroy(receive_buffer);

                        co_list_remove(self->tcp_clients, tcp_client);

                        return;
                    }
                    else
                    {
                        test_error("Failed: test_tcp_server_on_receive id error (%d)", header.id);
                        exit(-1);
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
}

static void test_tcp_server_on_close(test_tcp_server_thread_st* self, co_tcp_client_t* tcp_client)
{
    co_queue_destroy((co_queue_t*)co_tcp_get_user_data(tcp_client));

    co_list_remove(self->tcp_clients, tcp_client);

    if (self->close)
    {
        if (co_list_get_count(self->tcp_clients) == 0)
        {
            co_thread_t* parent = co_thread_get_parent((co_thread_t*)self);

            co_thread_send_event(parent, TEST_EVENT_TCP_SERVER_RES_CLOSE, 0, 0);

            test_info("tcp server send res-close");
        }
    }
}

static void test_tcp_server_on_tcp_accept(test_tcp_server_thread_st* self,
    co_tcp_server_t* tcp_server, co_tcp_client_t* tcp_client)
{
    assert(self->tcp_server == tcp_server);

    co_tcp_accept((co_thread_t*)self, tcp_client);

    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(tcp_client);
    callbacks->on_receive = (co_tcp_receive_fn)test_tcp_server_on_receive;
    callbacks->on_close = (co_tcp_close_fn)test_tcp_server_on_close;

    co_tcp_set_user_data(tcp_client, co_queue_create(1, NULL));

    co_list_add_tail(self->tcp_clients, tcp_client);
}

static void test_tcp_server_on_req_close(test_tcp_server_thread_st* self, const co_event_st* event)
{
    (void)event;

    co_assert(!self->close);

    test_info("tcp server receive req-close");

    self->close = true;

    if (co_list_get_count(self->tcp_clients) == 0)
    {
        co_thread_t* parent = co_thread_get_parent((co_thread_t*)self);

        co_thread_send_event(parent, TEST_EVENT_TCP_SERVER_RES_CLOSE, 0, 0);

        test_info("tcp server send res-close");
    }
}

static bool test_tcp_server_on_thread_create(test_tcp_server_thread_st* self)
{
    self->close = false;

    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_TCP_SERVER_REQ_CLOSE,
        (co_event_fn)test_tcp_server_on_req_close);

    // clients

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.destroy_value = (co_item_destroy_fn)co_tcp_client_destroy;
    self->tcp_clients = co_list_create(&list_ctx);

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, self->family);
    co_net_addr_set_port(&local_net_addr, self->port);

    // server

    self->tcp_server = co_tcp_server_create(&local_net_addr);

    if (self->tcp_server == NULL)
    {
        test_error("Failed: co_tcp_server_create(%d, %d)", self->family, self->port);
        exit(-1);
    }

    co_tcp_server_callbacks_st* callbacks = co_tcp_server_get_callbacks(self->tcp_server);
    callbacks->on_accept = (co_tcp_accept_fn)test_tcp_server_on_tcp_accept;

    co_socket_option_set_reuse_addr(
        co_tcp_server_get_socket(self->tcp_server), true);

    // server start

    if (!co_tcp_server_start(self->tcp_server, SOMAXCONN))
    {
        test_error("Failed: co_tcp_server_start(%d, %d)", self->family, self->port);
        exit(-1);
    }

    char msg[256];
    co_net_addr_to_string(&local_net_addr, msg, sizeof(msg));
    test_info("tcp server start: %s", msg);

    return true;
}

static void test_tcp_server_on_thread_destroy(test_tcp_server_thread_st* self)
{
    co_tcp_server_destroy(self->tcp_server);

    if (co_list_get_count(self->tcp_clients) != 0)
    {
        test_error("Failed: test_tcp_server_on_thread_destroy(%d, %d)", self->family, self->port);

        co_thread_set_exit_code(-1);
    }

    co_list_destroy(self->tcp_clients);
}

void test_tcp_server_thread_start(test_tcp_server_thread_st* test_tcp_server_thread)
{
    co_net_thread_setup(
        (co_thread_t*)test_tcp_server_thread, "test_tcp_server_thread",
        (co_thread_create_fn)test_tcp_server_on_thread_create,
        (co_thread_destroy_fn)test_tcp_server_on_thread_destroy);

    if (!co_thread_start((co_thread_t*)test_tcp_server_thread))
    {
        test_error("Failed: co_thread_start(test_tcp_server_thread)");
        exit(-1);
    }
}

void test_tcp_server_thread_stop(test_tcp_server_thread_st* test_tcp_server_thread)
{
    co_thread_stop((co_thread_t*)test_tcp_server_thread);
    co_thread_join((co_thread_t*)test_tcp_server_thread);
    co_net_thread_cleanup((co_thread_t*)test_tcp_server_thread);
}
