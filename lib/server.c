#include <string.h>
#include <stdlib.h>
#include <uv.h>
#include <gc.h>
#include <uuid4.h>

#include "include/server.h"
#include "include/log.h"
#include "include/internal/sanic_ascii.h"
#include "include/http_request.h"
#include "include/internal/request_handler.h"
#include "include/internal/server_internals.h"
#include "include/internal/middleware_handler.h"

uv_loop_t *loop;
uv_async_t middleware;
uv_async_t handler;

struct sanic_uv_data {
  char *req_id;
  char *addr_str;
  char *response_buffer;
  int conn_fd;
  struct sanic_http_request *req;
  struct sanic_http_response *res;
  const uv_tcp_t *client;
};

void alloc_buffer(uv_handle_t *, size_t suggested_size, uv_buf_t *buf) {
  buf->base = (char*) GC_MALLOC_ATOMIC_UNCOLLECTABLE(suggested_size);
  buf->len = suggested_size;
}

void sanic_cleanup_request(uv_handle_t*) {
  GC_gcollect_and_unmap();

#if GC_DEBUG
  GC_dump();

  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
#endif
}

void sanic_finish_write(uv_write_t *req, int status) {
  struct sanic_uv_data *data = req->data;

  if (status) {
    sanic_fmt_log_warn_req(data->req, "error writing to %s", data->addr_str);
  }

  const uv_tcp_t *client = data->client;
  GC_FREE(req);
  GC_FREE(data->response_buffer);
  GC_FREE(data->req);
  GC_FREE(data->res);
  GC_FREE(data);
  uv_close((uv_handle_t*) client, sanic_cleanup_request);
}

void sanic_finish_request(struct sanic_uv_data *data) {
  uv_write_t *req = (uv_write_t *) GC_MALLOC_UNCOLLECTABLE(sizeof(uv_write_t));
  req->data = data;
  size_t buff_len = sanic_calculate_response_size(data->res);

  char *buff = GC_MALLOC_ATOMIC_UNCOLLECTABLE(buff_len);
  data->response_buffer = buff;
  uv_buf_t write_buff = uv_buf_init(buff, buff_len);
  sanic_write_response(buff, data->req, data->res);

  uv_write(req, (uv_stream_t *) data->client, &write_buff, 1, sanic_finish_write);
}

void sanic_do_handler(uv_async_t* handle) {
  struct sanic_uv_data *data = handle->data;
  sanic_handle_connection_make_response(data->req, data->res);
  sanic_finish_request(data);
}

void sanic_handle_middleware(uv_async_t* handle) {
  struct sanic_uv_data *data = handle->data;
  struct sanic_proceed_or_reply ret = sanic_process_middlewares(data->req, data->res, data->addr_str);

  if (ret.reply) {
    sanic_finish_request(data);
  } else {
    handler.data = data;
    uv_async_send(&handler);
  }
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
    .conn_fd = data->conn_fd
  };

  if (n_read < 0) {
    if (n_read != UV_EOF) {
      sanic_fmt_log_warn_req(&init_req, "error reading from %s", data->addr_str);
      GC_FREE(data);
      uv_close((uv_handle_t*) client, NULL);
    }
  } else if (n_read > 0) {
    struct sanic_proceed_or_reply ret = sanic_handle_connection_read(buff, &init_req, data->addr_str);
    data->res = ret.res;

    if (ret.reply) {
      sanic_finish_request(data);
    } else {
      data->req = ret.req;

      if (middlewares == NULL) {
        handler.data = data;
        uv_async_send(&handler);
      } else {
        middleware.data = data;
        uv_async_send(&middleware);
      }
    }
  }

  if (buff->base) {
    GC_FREE(buff->base);
  }
}

void sanic_handle_new_connection(uv_stream_t *server, int status) {
  if (status < 0) {
    sanic_log_warn("server accept failed");
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
    data->conn_fd = client->loop->backend_fd;

    struct sanic_http_request tmp_request = {
      .req_id = data->req_id,
      .conn_fd = data->conn_fd
    };
    sanic_log_trace_req(&tmp_request, "accepted new connection");
    client->data = data;

    uv_read_start((uv_stream_t*)client, alloc_buffer, sanic_handle_read);
  } else {
    sanic_log_warn("server accept failed");
    uv_close((uv_handle_t*) client, NULL);
  }
}

void sanic_stop_serve_internal() {
  uv_stop(loop);
}

int sanic_http_serve(uint16_t port) {
  loop = uv_default_loop();
  uv_async_init(loop, &middleware, sanic_handle_middleware);
  uv_async_init(loop, &handler, sanic_do_handler);

  uv_tcp_t server;
  struct sockaddr_in sock_addr;
  uv_ip4_addr("127.0.0.1", port, &sock_addr);
  uv_os_fd_t fd;
  uv_tcp_simultaneous_accepts(&server, 1);

  sanic_setup_interrupts();

#if SANIC_SHOW_LOGO
  printf(sanic_ascii_logo);
#endif

  sanic_log_trace("initializing web server");

  uv_tcp_init_ex(loop, &server, AF_INET);
  uv_fileno((const uv_handle_t *) &server, &fd);
  sock_fd = fd;

  int flags = fcntl(sock_fd, F_GETFL, 0);
  fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

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

  if (uv_tcp_bind(&server, (struct sockaddr*) &sock_addr, 0) != 0) {
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