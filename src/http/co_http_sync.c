#include <coldforce/core/co_std.h>
#include <coldforce/core/co_string.h>

#include <coldforce/net/co_net_thread.h>

#include <coldforce/http/co_http_client.h>

//---------------------------------------------------------------------------//
// http sync
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

//---------------------------------------------------------------------------//
// private
//---------------------------------------------------------------------------//

typedef struct
{
    co_thread_t thread;

    char* base_url;
    co_net_addr_t local_net_addr;
    co_tls_ctx_st* tls_ctx;

    co_http_client_t* client;
    co_http_request_t* request;
    co_http_response_t* response;

    const char* save_file_name;
    FILE* fp;

} co_http_thread_t;

static bool
co_http_thread_on_receive_start(
    co_http_thread_t* self,
    co_http_client_t* client,
    const co_http_request_t* request,
    const co_http_response_t* response
)
{
    (void)client;
    (void)request;
    (void)response;

    co_assert(self->fp == NULL);

    self->fp = fopen(self->save_file_name, "wb");

    return (self->fp != NULL);
}

static bool
co_http_thread_on_receive_data(
    co_http_thread_t* self,
    co_http_client_t* client,
    const co_http_request_t* request,
    const co_http_response_t* response,
    const uint8_t* data_block,
    size_t data_block_size
)
{
    (void)client;
    (void)request;
    (void)response;

    co_assert(self->fp != NULL);

    return (fwrite(
        data_block, data_block_size, 1, self->fp) == 1);
}

static void
co_http_thread_on_receive_finish(
    co_http_thread_t* self,
    co_http_client_t* client,
    const co_http_request_t* request,
    co_http_response_t* response,
    int error_code
)
{
    (void)request;
    (void)error_code;

    client->response = NULL;
    self->response = response;

    co_thread_stop((co_thread_t*)self);
}

static void
co_http_thread_on_connect(
    co_http_thread_t* self,
    co_http_client_t* client,
    int error_code
)
{
    (void)client;

    if (error_code == 0)
    {
        if (co_http_send_request(
            self->client, self->request))
        {
            self->request = NULL;

            return;
        }
    }

    // error
    co_thread_stop((co_thread_t*)self);
}

static bool
co_http_thread_on_create(
    co_http_thread_t* self
)
{
    self->client = co_http_client_create(
        self->base_url, &self->local_net_addr, self->tls_ctx);

    if (self->client == NULL)
    {
        return false;
    }

    co_http_callbacks_st* callbacks =
        co_http_get_callbacks(self->client);

    callbacks->on_connect =
        (co_http_connect_fn)co_http_thread_on_connect;
    callbacks->on_receive_finish =
        (co_http_receive_finish_fn)co_http_thread_on_receive_finish;

    if (self->save_file_name != NULL)
    {
        callbacks->on_receive_start =
            (co_http_receive_start_fn)co_http_thread_on_receive_start;
        callbacks->on_receive_data =
            (co_http_receive_data_fn)co_http_thread_on_receive_data;
    }

    return co_http_connect(self->client);
}

static void
co_http_thread_on_destroy(
    co_http_thread_t* self
)
{
    co_http_request_destroy(self->request);
    co_http_client_destroy(self->client);
    co_string_destroy(self->base_url);

    if (self->fp != NULL)
    {
        fclose(self->fp);
    }
}

//---------------------------------------------------------------------------//
// public
//---------------------------------------------------------------------------//

co_http_response_t*
co_http_sync_request(
    const char* url,
    co_http_request_t* request,
    const char* save_file_name,
    const co_net_addr_t* local_net_addr,
    co_tls_ctx_st* tls_ctx
)
{
    co_http_thread_t http_thread = { 0 };

    co_url_st* http_url = co_url_create(url);

    http_thread.base_url = co_url_create_base_url(http_url);

    if (co_http_request_get_path(request) == NULL)
    {
        char* path = co_url_create_path_and_query(http_url);
        co_http_request_set_path(request, path);
        co_string_destroy(path);
    }

    co_url_destroy(http_url);

    if (co_http_request_get_version(request) == NULL)
    {
        co_http_request_set_version(request, CO_HTTP_VERSION_1_1);
    }

    if (local_net_addr != NULL)
    {
        memcpy(&http_thread.local_net_addr,
            local_net_addr, sizeof(co_net_addr_t));
    }
    else
    {
        co_net_addr_set_family(
            &http_thread.local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
    }

    http_thread.request = request;
    http_thread.save_file_name = save_file_name;
    http_thread.tls_ctx = tls_ctx;

    co_net_thread_setup(
        (co_thread_t*)&http_thread,
        (co_thread_create_fn)co_http_thread_on_create,
        (co_thread_destroy_fn)co_http_thread_on_destroy);

    // thread start
    co_thread_start((co_thread_t*)&http_thread);

    // wait
    co_thread_wait((co_thread_t*)&http_thread);
    co_net_thread_cleanup((co_thread_t*)&http_thread);

    return http_thread.response;
}
