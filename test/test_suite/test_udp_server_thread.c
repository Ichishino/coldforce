#include "test_udp_server_thread.h"
#include "test_app.h"

static void test_udp_server_on_receive(test_udp_server_thread_st* self, co_udp_t* udp)
{
    (void)self;

    for (;;)
    {
        char data[1024];

        co_net_addr_t remote_net_addr;
        ssize_t size = co_udp_receive(udp, &remote_net_addr, data, sizeof(data));

        if (size <= 0)
        {
            break;
        }
        
        if (!co_udp_send(udp, &remote_net_addr, data, size))
        {
            test_error("Failed: test_udp_server_on_receive(co_udp_send)");
            exit(-1);
        }
    }
}

static bool test_udp_server_thread_on_create(test_udp_server_thread_st* self)
{
    co_net_addr_t local_net_addr = { 0 };
    co_net_addr_set_family(&local_net_addr, self->family);
    co_net_addr_set_port(&local_net_addr, self->port);

    self->udp_server = co_udp_create(&local_net_addr);

    if (self->udp_server == NULL)
    {
        return false;
    }

    co_udp_callbacks_st* callbacks = co_udp_get_callbacks(self->udp_server);
    callbacks->on_receive = (co_udp_receive_fn)test_udp_server_on_receive;

    co_socket_option_set_reuse_addr(
        co_udp_get_socket(self->udp_server), true);

    if (!co_udp_receive_start(self->udp_server))
    {
        test_error("Failed: test_udp_server_thread_on_create(%d, %d)", self->family, self->port);
        exit(-1);
    }

    char msg[256];
    co_net_addr_to_string(&local_net_addr, msg, sizeof(msg));
    test_info("udp server start: %s", msg);

    return true;
}

static void test_udp_server_thread_on_destroy(test_udp_server_thread_st* self)
{
    co_udp_destroy(self->udp_server);
}

void test_udp_server_thread_start(test_udp_server_thread_st* test_udp_server_thread)
{
    co_net_thread_setup(
        (co_thread_t*)test_udp_server_thread, "test_udp_server_thread",
        (co_thread_create_fn)test_udp_server_thread_on_create,
        (co_thread_destroy_fn)test_udp_server_thread_on_destroy);

    if (!co_thread_start((co_thread_t*)test_udp_server_thread))
    {
        test_error("Failed: co_thread_start(test_udp_server_thread_start)");
        exit(-1);
    }
}

void test_udp_server_thread_stop(test_udp_server_thread_st* test_udp_server_thread)
{
    co_thread_stop((co_thread_t*)test_udp_server_thread);
    co_thread_join((co_thread_t*)test_udp_server_thread);
    co_net_thread_cleanup((co_thread_t*)test_udp_server_thread);
}
