
# Coldforce

[![Build](https://github.com/Ichishino/coldforce/actions/workflows/build.yml/badge.svg)](https://github.com/Ichishino/coldforce/actions/workflows/build.yml)

Coldforce is a library written in C that supports various network protocols.
With the asynchronous API of this library
You can easily develop event-driven network applications.
The currently supported protocols are as follows.
All of these support clients and servers (multi-client, C10K).

* TCP/UDP (IPv4/IPv6)
* TLS/DTLS (OpenSSL or wolfSSL) (Sorry, DTLS doesn't work on windows)
* HTTP/1.1 (http/https, pipelining, basic/digest authentication)
* HTTP/2 (server push)
* WebSocket (ws/wss, over http2)

## Platforms

* Windows
* Linux
* macOS

## Requirements

* C99 or later
* Use `-pthread` `-lm`
* OpenSSL or wolfSSL (only when using TLS, https and wss)

  wolfSSL build options

  ```shellsession
  IDE

  #define OPENSSL_EXTRA
  #define OPENSSL_ALL
  #define HAVE_ALPN
  #define HAVE_SNI
  #define WOLFSSL_SYS_CA_CERTS
  #define WOLFSSL_DTLS
  #define WOLFSSL_DTLS13

  mkdir inc/wolfssl
  copy your user_settings.h to inc/wolfssl/.
  ```

  ```shellsession
  configure

  --enable-opensslextra --enable-opensslall --enable-alpn --enable-sni --enable-sys-ca-certs --enable-dtls --enable-dtls13
  ```

## Modules

* Coldforce core : `co_core.dll` / `libco_core.a`
* Network core (TCP,UDP) : `co_net.dll` / `libco_net.a`
* TLS/DTLS : `co_tls.dll` / `libco_tls.a`
* HTTP/1.1 : `co_http.dll` / `libco_http.a`
* HTTP/2 : `co_http2.dll` / `libco_http2.a`
* WebSocket : `co_ws.dll`, `co_ws_http2.dll` / `libco_ws.a`, `libco_ws_http2.a`

## Builds

* Windows
Visual Studio ([prj/msvc/coldforce.sln](https://github.com/Ichishino/coldforce/tree/master/prj/msvc))

  for wolfSSL
  Add `CO_USE_WOLFSSL` to `C/C++ Preprocessor Definitions` in both co_tls and your project property.

* Linux
  cmake

  ```shellsession
  cd build
  cmake ..
  make
  ```

  for wolfSSL

  ```shellsession
  ...
  cmake .. -DTLS_LIB=wolfssl
  ...
  ```

* macOS
  cmake (same way as Linux)

## Code Examples

* WebSocket client

  ```C++
  #include <coldforce.h>

  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

  typedef struct {
      co_app_t base_app;
      co_ws_client_t* ws_client;
      co_url_st* url;
  } app_st;

  void
  app_on_ws_receive_frame(
      app_st* self,
      co_ws_client_t* ws_client,
      const co_ws_frame_t* frame,
      int error_code
  )
  {
      if (error_code == 0)
      {
          bool fin = co_ws_frame_get_fin(frame);
          uint8_t opcode = co_ws_frame_get_opcode(frame);
          size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
          const uint8_t* data = co_ws_frame_get_payload_data(frame);

          switch (opcode)
          {
          case CO_WS_OPCODE_TEXT:
          {
              printf("receive text(%d): %*.*s\n", fin,
                  (int)data_size, (int)data_size, (char*)data);
              break;
          }
          case CO_WS_OPCODE_BINARY:
          {
              printf("receive binary(%d): %zu bytes\n",
                  fin, data_size);
              break;
          }
          case CO_WS_OPCODE_CONTINUATION:
          {
              printf("receive continuation(%d): %zu bytes\n",
                  fin, data_size);
              break;
          }
          default:
          {
              co_ws_default_handler(ws_client, frame);
              break;
          }
          }
      }
      else
      {
          co_ws_client_destroy(ws_client);
          self->ws_client = NULL;
      }
  }

  void
  app_on_ws_close(
      app_st* self,
      co_ws_client_t* ws_client
  )
  {
      co_ws_client_destroy(ws_client);
      self->ws_client = NULL;

      co_app_stop();
  }

  void
  app_on_ws_upgrade(
      app_st* self,
      co_ws_client_t* ws_client,
      const co_http_response_t* response,
      int error_code
  )
  {
      if (error_code == 0)
      {
          co_ws_send_text(ws_client, "hello");
      }
      else
      {
          co_ws_client_destroy(ws_client);
          self->ws_client = NULL;

          co_app_stop();
      }
  }

  void
  app_on_ws_connect(
      app_st* self,
      co_ws_client_t* ws_client,
      int error_code
  )
  {
      if (error_code == 0)
      {
          co_http_request_t* request =
              co_http_request_create_ws_upgrade(self->url->path_and_query, NULL, NULL);

          co_ws_send_upgrade_request(self->ws_client, request);
      }
      else
      {
          co_ws_client_destroy(ws_client);
          self->ws_client = NULL;

          co_app_stop();
      }
  }

  bool
  app_on_create(
      app_st* self
  )
  {
      self->url = co_url_create("ws://127.0.0.1:8080/");

      co_net_addr_t local_net_addr = { 0 };
      co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);

      self->ws_client = co_ws_client_create(self->url->origin, &local_net_addr, NULL);

      if (self->ws_client == NULL)
      {
          return false;
      }

      co_ws_callbacks_st* callbacks = co_ws_get_callbacks(self->ws_client);
      callbacks->on_connect = (co_ws_connect_fn)app_on_ws_connect;
      callbacks->on_upgrade = (co_ws_upgrade_fn)app_on_ws_upgrade;
      callbacks->on_receive_frame = (co_ws_receive_frame_fn)app_on_ws_receive_frame;
      callbacks->on_close = (co_ws_close_fn)app_on_ws_close;

      co_ws_connect_start(self->ws_client);

      return true;
  }

  void
  app_on_destroy(
      app_st* self
  )
  {
      co_ws_client_destroy(self->ws_client);
      co_url_destroy(self->url);
  }

  int
  main(
      int argc,
      char* argv[]
  )
  {
      app_st self = { 0 };

      return co_net_app_start(
          (co_app_t*)&self, "ws-client-app",
          (co_app_create_fn)app_on_create,
          (co_app_destroy_fn)app_on_destroy,
          argc, argv);
  }
  ```

* WebSocket echo server -> ws://127.0.0.1:8080

  ```C++
  #include <coldforce.h>

  typedef struct {
      co_app_t base_app;
      co_tcp_server_t* tcp_server;
      co_list_t* ws_clients;
  } app_st;

  void
  app_on_ws_receive_frame(
      app_st* self,
      co_ws_client_t* ws_client,
      const co_ws_frame_t* frame,
      int error_code
  )
  {
      if (error_code == 0)
      {
          bool fin = co_ws_frame_get_fin(frame);
          uint8_t opcode = co_ws_frame_get_opcode(frame);
          size_t data_size = (size_t)co_ws_frame_get_payload_size(frame);
          const uint8_t* data = co_ws_frame_get_payload_data(frame);

          switch (opcode)
          {
          case CO_WS_OPCODE_TEXT:
          case CO_WS_OPCODE_BINARY:
          case CO_WS_OPCODE_CONTINUATION:
          {
              co_ws_send(ws_client, fin, opcode, data, data_size);

              break;
          }
          default:
          {
              co_ws_default_handler(ws_client, frame);

              break;
          }
          }
      }
      else
      {
          co_list_remove(self->ws_clients, ws_client);
      }
  }

  void
  app_on_ws_close(
      app_st* self,
      co_ws_client_t* ws_client
  )
  {
      co_list_remove(self->ws_clients, ws_client);
  }

  void
  app_on_ws_upgrade(
      app_st* self,
      co_ws_client_t* ws_client,
      const co_http_request_t* http_request,
      int error_code
  )
  {
      if (error_code == 0)
      {
          co_http_response_t* http_response =
              co_http_response_create_ws_upgrade(http_request, NULL, NULL);

          co_http_connection_send_response(
              (co_http_connection_t*)ws_client, http_response);

          co_http_response_destroy(http_response);
      }
      else
      {
          co_list_remove(self->ws_clients, ws_client);
      }
  }

  void
  app_on_tcp_accept(
      app_st* self,
      co_tcp_server_t* tcp_server,
      co_tcp_client_t* tcp_client
  )
  {
      co_tcp_accept((co_thread_t*)self, tcp_client);

      co_ws_client_t* ws_client = co_tcp_upgrade_to_ws(tcp_client, NULL);

      co_ws_callbacks_st* callbacks = co_ws_get_callbacks(ws_client);
      callbacks->on_upgrade = (co_ws_upgrade_fn)app_on_ws_upgrade;
      callbacks->on_receive_frame = (co_ws_receive_frame_fn)app_on_ws_receive_frame;
      callbacks->on_close = (co_ws_close_fn)app_on_ws_close;

      co_list_add_tail(self->ws_clients, ws_client);
  }

  bool
  app_on_create(
    app_st* self
  )
  {
      uint16_t port = 8080;

      co_list_ctx_st list_ctx = { 0 };
      list_ctx.destroy_value = (co_item_destroy_fn)co_ws_client_destroy;
      self->ws_clients = co_list_create(&list_ctx);

      co_net_addr_t local_net_addr = { 0 };
      co_net_addr_set_family(&local_net_addr, CO_NET_ADDR_FAMILY_IPV4);
      co_net_addr_set_port(&local_net_addr, port);

      self->tcp_server = co_tcp_server_create(&local_net_addr);

      co_socket_option_set_reuse_addr(
          co_tcp_server_get_socket(self->tcp_server), true);

      co_tcp_server_callbacks_st* callbacks =
          co_tcp_server_get_callbacks(self->tcp_server);
      callbacks->on_accept = (co_tcp_accept_fn)app_on_tcp_accept;

      return co_tcp_server_start(self->tcp_server, SOMAXCONN);
  }

  void
  app_on_destroy(
      app_st* self
  )
  {
      co_tcp_server_destroy(self->tcp_server);
      co_list_destroy(self->ws_clients);
  }

  int
  main(
      int argc,
      char* argv[]
  )
  {
      app_st self = { 0 };

      return co_net_app_start(
          (co_app_t*)&self, "ws-server-app",
          (co_app_create_fn)app_on_create,
          (co_app_destroy_fn)app_on_destroy,
          argc, argv);
  }
  ```

* [more examples here](https://github.com/Ichishino/coldforce/tree/master/examples)
