#include "test_tcp_server_comm.h"
#include "test_tcp_server.h"
#include "test_app.h"

static void
test_tcp_server_on_receive(
    test_tcp_server_thread_st* self,
    co_tcp_client_t* tcp_client
)
{
    for (;;)
    {
        char data[1024];

        ssize_t size =
            co_tcp_receive(tcp_client, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }

        co_queue_t* receive_buffer =
            (co_queue_t*)co_tcp_get_user_data(tcp_client);

        co_queue_push_array(receive_buffer, data, size);

        while (co_queue_get_count(receive_buffer) >=
            TEST_TCP_PACKET_HEADER_SIZE)
        {
            test_tcp_packet_header_st header;
            co_queue_peek_array(
                receive_buffer, &header, TEST_TCP_PACKET_HEADER_SIZE);

            header.id =
                co_byte_order_32_network_to_host(header.id);
            header.size =
                co_byte_order_32_network_to_host(header.size);

            size_t packet_size =
                header.size + TEST_TCP_PACKET_HEADER_SIZE;

            if (co_queue_get_count(receive_buffer) < packet_size)
            {
                break;
            }

            if (header.id == 1)
            {
                char send_data[2048];
                co_queue_pop_array(
                    receive_buffer, send_data, packet_size);

                co_tcp_send(
                    tcp_client,
                    &send_data[TEST_TCP_PACKET_HEADER_SIZE], header.size);
            }
            else if (header.id == 2)
            {
                co_queue_destroy(receive_buffer);

                co_list_remove(self->thread.clients, tcp_client);

                return;
            }
            else
            {
                test_error(
                    "Failed: test_tcp_server_on_receive id error (%d)",
                    header.id);

                exit(-1);
            }
        }
    }
}

static void
test_tcp_server_on_close(
    test_tcp_server_thread_st* self,
    co_tcp_client_t* tcp_client
)
{
    co_queue_destroy(
        (co_queue_t*)co_tcp_get_user_data(tcp_client));

    co_list_remove(self->thread.clients, tcp_client);

    if (self->close)
    {
        if (co_list_get_count(self->thread.clients) == 0)
        {
            co_thread_t* parent =
                co_thread_get_parent((co_thread_t*)self);

            co_thread_send_event(
                parent, TEST_EVENT_TCP_SERVER_RES_CLOSE, 0, 0);

            test_info("tcp server send res-close");
        }
    }
}

void
test_tcp_server_on_accept(
    test_tcp_server_thread_st* self,
    co_tcp_server_t* tcp_server,
    co_tcp_client_t* tcp_client
)
{
    co_assert(self->tcp_server == tcp_server);

    co_tcp_accept((co_thread_t*)self, tcp_client);

    co_tcp_callbacks_st* callbacks = co_tcp_get_callbacks(tcp_client);
    callbacks->on_receive = (co_tcp_receive_fn)test_tcp_server_on_receive;
    callbacks->on_close = (co_tcp_close_fn)test_tcp_server_on_close;

    co_tcp_set_user_data(tcp_client, co_queue_create(1, NULL));

    co_list_add_tail(self->thread.clients, tcp_client);
}
