#include <string.h>
#include <stdlib.h>
#include <uv.h>
#include <gc.h>
#include <uuid4.h>

#include "include/server.h"
#include "include/log.h"
#include "include/internal/sanic_ascii.h"
#include "include/internal/server_internals.h"
#include "include/http_request.h"
#include "include/internal/request_handler.h"

uv_loop_t *loop;
uv_async_t *middleware;
uv_async_t *handler;

struct sanic_uv_data {
  char *req_id;
  char *addr_str;
  struct sanic_http_request *req;
  struct sanic_http_response *res;
  const uv_tcp_t *client;
};

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
  buf->base = (char*) malloc(suggested_size);
  buf->len = suggested_size;
}

void echo_write(uv_write_t *req, int status) {
  if (status) {
    fprintf(stderr, "Write error %s\n", uv_strerror(status));
  }
  free(req);
}

void sanic_handle_read(uv_stream_t *client, ssize_t n_read, const uv_buf_t *buff) {
  struct sanic_uv_data *data = client->data;
  char addr_str[INET_ADDRSTRLEN];
  struct sockaddr_in conn_addr;
  int len = sizeof(struct sockaddr_in);

  uv_tcp_getsockname(data->client, (struct sockaddr *)&conn_addr, &len);
  inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
  data->addr_str = GC_STRDUP(addr_str);

  struct sanic_http_request init_req = {
    .req_id = data->req_id,
    .conn_fd = client->accepted_fd
  };

  if (n_read < 0) {
    if (n_read != UV_EOF) {
      sanic_fmt_log_warn_req(&init_req, "error reading from %s", data->addr_str);
      uv_close((uv_handle_t*) client, NULL);
    }
  } else if (n_read > 0) {
    sanic_handle_connection_read(buff, &init_req, data->addr_str);

    uv_write_t *req = (uv_write_t *) malloc(sizeof(uv_write_t));
    uv_buf_t write_buff = uv_buf_init(buff->base, n_read);
    uv_write(req, client, &write_buff, 1, echo_write);
  }

  if (buff->base) {
    free(buff->base);
  }
}

void sanic_handle_new_connection(uv_stream_t *server, int status) {
  if (status < 0) {
    fprintf(stderr, "New connection error %s\n", uv_strerror(status));
    return;
  }

  uv_tcp_t *client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));

  struct sanic_uv_data *data = GC_MALLOC_UNCOLLECTABLE(sizeof(struct sanic_uv_data));
  data->client = client;

  char req_id[UUID4_LEN] = {0};
  bzero(req_id, 37);
  uuid4_generate(req_id);
  data->req_id = GC_STRDUP(req_id);

  uv_tcp_init(loop, client);
  if (uv_accept(server, (uv_stream_t*) client) == 0) {
    struct sanic_http_request tmp_request = {
      .req_id = data->req_id,
      .conn_fd = client->accepted_fd
    };
    sanic_log_trace_req(&tmp_request, "accepted new connection");
    client->data = data;

    uv_read_start((uv_stream_t*)client, alloc_buffer, sanic_handle_read);
  } else {
    sanic_log_warn("server accept failed");
    uv_close((uv_handle_t*) client, NULL);
  }
}

int sanic_http_serve(uint16_t port) {
  loop = uv_default_loop();
  uv_tcp_t server;

  sanic_setup_interrupts();

#if SANIC_SHOW_LOGO
  printf(sanic_ascii_logo);
#endif

  sanic_log_trace("initializing web server");

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    sanic_fmt_log_error("socket creation failed: %s", strerror(errno));
    return 1;
  }
  sanic_log_debug("socket successfully created");

  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
    sanic_fmt_log_error("socket creation failed: %s", strerror(errno));
    return 1;
  }

  struct linger l = {
    .l_onoff = 1,
    .l_linger = 0
  };

  if (setsockopt(sock_fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) != 0) {
    sanic_log_error("setting SO_LINGER failed");
    return 1;
  }

  struct sockaddr_in sock_addr = {
    .sin_family = AF_INET,
    .sin_addr.s_addr = htonl(INADDR_ANY),
    .sin_port = htons(port)
  };

  if (uv_tcp_bind(&server, (struct sockaddr *) &sock_addr, 0) != 0) {
    sanic_fmt_log_error("socket bind failed: %s", strerror(errno));
    return 1;
  }
  sanic_fmt_log_debug("socket bind to port %d successful", port);

  if (uv_listen((uv_stream_t*)&server, 128, sanic_handle_new_connection)) {
    sanic_fmt_log_error("listen on port %d failed: %s", port, strerror(errno));
    return 1;
  }
  sanic_fmt_log_info("server listening on port %d", port);

  return uv_run(loop, UV_RUN_DEFAULT);
}