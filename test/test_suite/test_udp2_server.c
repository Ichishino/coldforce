#include "test_udp2_server.h"
#include "test_app.h"

static void test_udp2_server_on_client_receive(test_udp2_server_thread_st* self, co_udp_t* udp_client)
{
    co_assert(self->udp_server != udp_client);

    uint16_t port;
    co_net_addr_get_port(
        co_socket_get_remote_net_addr(co_udp_get_socket(
            udp_client)), &port);

    for (;;)
    {
        char data[2048];
        uint16_t remote_port;
// TODO
#if 0
        ssize_t size = co_udp_receive(udp_client, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }
#else
        co_net_addr_t remote_net_addr;
        ssize_t size = co_udp_receive_from(udp_client, &remote_net_addr, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }
        
        co_net_addr_get_port(&remote_net_addr, &remote_port);
        
        if (port != remote_port)
        {
            test_info("----- %d %d", port, remote_port);
            
            co_udp_send_to(udp_client, &remote_net_addr, data, size);
        
            continue;
        }
#endif
        test_udp_packet_header_st* header = (test_udp_packet_header_st*)data;

        if (header->close != 0)
        {
            co_list_remove(self->udp_clients, udp_client);

            if (self->close)
            {
                if (co_list_get_count(self->udp_clients) == 0)
                {
                    co_thread_t* parent = co_thread_get_parent((co_thread_t*)self);

                    co_thread_send_event(parent, TEST_EVENT_UDP2_SERVER_RES_CLOSE, 0, 0);

                    test_info("udp2 server send res-close");
                }
            }

            break;
        }
        
        remote_port = (uint16_t)co_byte_order_32_network_to_host((uint32_t)header->port);
        
        if (port != remote_port)
        {
            test_error("Failed: test_udp2_server_on_client_receive(port %d, %d)", port, remote_port);
            exit(-1);
        }        

        if (!co_udp_send(udp_client, data, size))
        {
            test_error("Failed: test_udp2_server_on_client_receive(co_udp_send)");
            exit(-1);
        }
    }
}

static void test_udp2_server_on_server_receive(test_udp2_server_thread_st* self, co_udp_t* udp_server)
{
    co_assert(self->udp_server == udp_server);

    for (;;)
    {
        char data[2048];

        co_net_addr_t remote_net_addr;
        ssize_t size = co_udp_receive_from(udp_server, &remote_net_addr, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }

        co_udp_t* udp_client =
            co_udp_create_connection(udp_server, &remote_net_addr);
            
        if (udp_client == NULL)
        {
            char remote_str[256];
            co_net_addr_to_string(&remote_net_addr, remote_str, sizeof(remote_str));

            test_error("Failed: test_udp2_server_on_server_receive(co_udp_create_connection(%s))", remote_str);
            exit(-1);        
        }            

        if (co_list_find(self->udp_clients, udp_client) != NULL)
        {
            char remote_str[256];
            co_net_addr_to_string(&remote_net_addr, remote_str, sizeof(remote_str));

            test_error("Failed: test_udp2_server_on_server_receive(co_list_find(%s))", remote_str);
            exit(-1);
        }

        if (!co_udp_accept((co_thread_t*)self, udp_client))
        {
            test_error("Failed: test_udp2_server_on_server_receive(co_udp_accept)");
            exit(-1);
        }
        
        uint16_t port;
        co_net_addr_get_port(
            co_socket_get_remote_net_addr(co_udp_get_socket(
                udp_client)), &port);
        test_info("udp accept %d", port);        

        co_list_add_tail(self->udp_clients, udp_client);

        co_udp_callbacks_st* callbacks = co_udp_get_callbacks(udp_client);
        callbacks->on_receive = (co_udp_receive_fn)test_udp2_server_on_client_receive;

        if (!co_udp_send(udp_client, data, size))
        {
            test_error("Failed: test_udp2_server_on_server_receive(co_udp_send)");
            exit(-1);
        }
    }
}

static void test_udp2_server_on_req_close(test_udp2_server_thread_st* self, const co_event_st* event)
{
    (void)event;

    co_assert(!self->close);

    test_info("udp2 server receive req-close");

    self->close = true;

    if (co_list_get_count(self->udp_clients) == 0)
    {
        co_thread_t* parent = co_thread_get_parent((co_thread_t*)self);

        co_thread_send_event(parent, TEST_EVENT_UDP2_SERVER_RES_CLOSE, 0, 0);

        test_info("udp2 server send res-close");
    }
}

static int test_udp2_server_thread_compare_clients(const co_udp_t* udp1, const co_udp_t* udp2)
{
    const co_socket_t* sock1 = co_udp_get_socket((co_udp_t*)udp1);
    const co_socket_t* sock2 = co_udp_get_socket((co_udp_t*)udp2);

    const co_net_addr_t* remote_net_addr1 = co_socket_get_remote_net_addr(sock1);
    const co_net_addr_t* remote_net_addr2 = co_socket_get_remote_net_addr(sock2);

    return co_net_addr_is_equal(remote_net_addr1, remote_net_addr2) ? 0 : 1;
}

static bool test_udp2_server_thread_on_create(test_udp2_server_thread_st* self)
{
    co_thread_set_event_handler(
        (co_thread_t*)self,
        TEST_EVENT_UDP2_SERVER_REQ_CLOSE,
        (co_event_fn)test_udp2_server_on_req_close);

    co_list_ctx_st list_ctx = { 0 };
    list_ctx.compare_values = (co_item_compare_fn)test_udp2_server_thread_compare_clients;
    list_ctx.destroy_value = (co_item_destroy_fn)co_udp_destroy;
    self->udp_clients = co_list_create(&list_ctx);

    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, self->family);

    if (self->family == CO_NET_ADDR_FAMILY_UNIX)
    {
        co_net_addr_set_unix_path(&local_net_addr, self->address);
    }
    else
    {
        co_net_addr_set_port(&local_net_addr, self->port);
    }

    self->udp_server = co_udp_create(&local_net_addr);

    if (self->udp_server == NULL)
    {
        return false;
    }

    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(self->udp_server);
    callbacks->on_receive = (co_udp_receive_fn)test_udp2_server_on_server_receive;

    co_socket_option_set_reuse_addr(
        co_udp_get_socket(self->udp_server), true);

    if (!co_udp_receive_start(self->udp_server))
    {
        test_error("Failed: test_udp2_server_thread_on_create(%d, %d)", self->family, self->port);
        exit(-1);
    }

    char msg[256];
    co_net_addr_to_string(&local_net_addr, msg, sizeof(msg));
    test_info("udp2 server start: %s", msg);

    return true;
}

static void test_udp2_server_thread_on_destroy(test_udp2_server_thread_st* self)
{
    co_udp_destroy(self->udp_server);
    co_list_destroy(self->udp_clients);
}

void test_udp2_server_thread_start(test_udp2_server_thread_st* test_udp2_server_thread)
{
    co_net_thread_setup(
        (co_thread_t*)test_udp2_server_thread, "test_udp2_server_thread",
        (co_thread_create_fn)test_udp2_server_thread_on_create,
        (co_thread_destroy_fn)test_udp2_server_thread_on_destroy);

    if (!co_thread_start((co_thread_t*)test_udp2_server_thread))
    {
        test_error("Failed: co_thread_start(test_udp2_server_thread_start)");
        exit(-1);
    }
}

void test_udp2_server_thread_stop(test_udp2_server_thread_st* test_udp2_server_thread)
{
    co_thread_stop((co_thread_t*)test_udp2_server_thread);
    co_thread_join((co_thread_t*)test_udp2_server_thread);
    co_net_thread_cleanup((co_thread_t*)test_udp2_server_thread);
}
