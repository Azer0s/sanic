#if SANIC_IO_URING

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <liburing.h>
#include <uuid4.h>
#include <gc.h>
#include <fcntl.h>

#include "include/log.h"
#include "include/internal/server_internals.h"
#include "include/http_request.h"
#include "include/internal/request_handler.h"
#include "include/internal/middleware_handler.h"

enum sanic_iouring_type {
  ACCEPT,
  READ,
  WRITE,
  MIDDLEWARE,
  HANDLER
};

struct sanic_iouring_connection_data {
  char *req_id;
  char *addr_str;
  int conn_fd;
  struct sanic_http_request *req;
  struct sanic_http_response *res;
  enum sanic_iouring_type type;
};

struct io_uring ring;

void sanic_stop_serve_internal() {
  io_uring_queue_exit(&ring);
}

char *sanic_sig2str(int signum) {
  return strsignal(signum);
}

int add_accept_request(struct sockaddr_in *client_addr, socklen_t *client_addr_len) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  io_uring_prep_accept(sqe, sock_fd, (struct sockaddr *) client_addr, client_addr_len, 0);
  struct sanic_iouring_connection_data *data = GC_MALLOC_UNCOLLECTABLE(sizeof(struct sanic_iouring_connection_data));
  data->type = ACCEPT;
  io_uring_sqe_set_data(sqe, data);
  io_uring_submit(&ring);
  return 0;
}

int add_next_step(int conn_fd, enum sanic_iouring_type type, struct sanic_iouring_connection_data *data) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  data->type = type;
  data->conn_fd = conn_fd;
  io_uring_sqe_set_data(sqe, data);
  io_uring_submit(&ring);
  return 0;
}

int sanic_http_serve(uint16_t port) {

  sanic_log_debug("initializing io_uring");
  io_uring_queue_init(10, &ring, 0);

  struct io_uring_cqe *cqe;
  struct sockaddr_in conn_addr;
  socklen_t len = sizeof(struct sockaddr_in);

  add_accept_request(&conn_addr, &len);

  while (!stop) {
    int ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
      sanic_log_error("io_uring_wait_cqe");
      return 1;
    }

    struct sanic_iouring_connection_data *data = (struct sanic_iouring_connection_data *) cqe->user_data;
    if (cqe->res < 0) {
      sanic_fmt_log_error("Async request failed: %s for event: %d\n", strerror(-cqe->res), data->type);
      exit(1);
    }

    switch (data->type) {
      case ACCEPT: {
        add_accept_request(&conn_addr, &len);

        int conn_fd = accept(sock_fd, (struct sockaddr *) &conn_addr, (socklen_t *) &len);

        char req_id[UUID4_LEN] = {0};
        bzero(req_id, 37);
        uuid4_generate(req_id);

        struct sanic_http_request tmp_request = {
          .req_id = req_id,
          .conn_fd = conn_fd
        };

        sanic_log_trace_req(&tmp_request, "accepted new connection");
        if (conn_fd < 0) {
          sanic_log_warn("server accept failed");
          continue;
        }

        char addr_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);

        data->req_id = req_id;
        data->conn_fd = conn_fd;
        data->addr_str = GC_STRDUP(addr_str);

        add_next_step(conn_fd, READ, data);
      }
        break;
      case READ: {
        struct sanic_http_request tmp_request = {
          .req_id = data->req_id,
          .conn_fd = data->conn_fd
        };
        struct sanic_proceed_or_reply proceed = sanic_handle_connection_read(&tmp_request, data->addr_str);
        data->res = proceed.res;

        if (proceed.reply) {
          add_next_step(data->conn_fd, WRITE, data);
        } else {
          data->req = proceed.req;
          add_next_step(data->conn_fd, MIDDLEWARE, data);
        }
      } break;
      case WRITE: {
        sanic_finish_request(data->req, data->res, data->addr_str);
        GC_FREE(data);
        GC_gcollect_and_unmap();
      } break;
      case MIDDLEWARE: {
        struct sanic_proceed_or_reply proceed = sanic_handle_middlewares(data->req, data->res, data->addr_str);
        if (proceed.reply) {
          data->res = proceed.res;
          add_next_step(data->conn_fd, WRITE, data);
        } else {
          add_next_step(data->conn_fd, HANDLER, data);
        }
      } break;
      case HANDLER: {
        sanic_handle_connection_make_response(data->req, data->res);
        add_next_step(data->conn_fd, WRITE, data);
      } break;
    }

    io_uring_cqe_seen(&ring, cqe);
  }
}

#endif
