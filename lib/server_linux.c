#if SANIC_IO_URING

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <liburing.h>
#include <string.h>
#include <gc.h>

#include "include/log.h"
#include "include/http_request.h"
#include "include/internal/server_internals.h"
#include "include/internal/request_handler.h"

struct io_uring ring;

enum sanic_io_uring_event_type {
  EVENT_TYPE_ACCEPT,
  EVENT_TYPE_READ,
  EVENT_TYPE_WRITE,
};

struct sanic_io_uring_request {
  enum sanic_io_uring_event_type event_type;
  char *req_id;
  struct sockaddr_in *conn_addr;

  int iovec_count;
  int client_socket;
  struct iovec iov[];
};

int add_accept_request(int server_socket, struct sockaddr_in *client_addr, socklen_t *client_addr_len) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  io_uring_prep_accept(sqe, server_socket, (struct sockaddr *) client_addr,
                       client_addr_len, 0);
  struct sanic_io_uring_request *req = malloc(sizeof(struct sanic_io_uring_request));
  req->event_type = EVENT_TYPE_ACCEPT;
  io_uring_sqe_set_data(sqe, req);
  io_uring_submit(&ring);
  return 0;
}

void stop_uring() {
  io_uring_queue_exit(&ring);
}

int sanic_http_serve(uint16_t port) {
  sanic_setup_interrupts(stop_uring, strsignal);
  int code = sanic_create_socket(port);

  if (code != 0) {
    return code;
  }

  sanic_log_debug("initializing io_uring");

  io_uring_queue_init(10, &ring, 0);

  struct sockaddr_in conn_addr;
  socklen_t len = sizeof(conn_addr);

  add_accept_request(sock_fd, &conn_addr, &len);

  while (!stop) {
    struct io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&ring, &cqe);
    struct sanic_io_uring_request *uring_req = (struct sanic_io_uring_request*) cqe->user_data;

    if (ret < 0) {
      //TODO: log error and fail
      return 1;
    }

    if (cqe->res < 0) {
      //TODO: log error
      return 1;
    }

    switch (uring_req->event_type) {
      case EVENT_TYPE_ACCEPT: {
        struct sanic_http_request tmp_request = {
          .req_id = uring_req->req_id,
          .conn_fd = uring_req->client_socket
        };

        free(uring_req);
        add_accept_request(sock_fd, &conn_addr, &len);

        sanic_log_trace_req(&tmp_request, "accepted new connection");
        if (tmp_request.conn_fd < 0) {
          sanic_log_warn("server accept failed");
          continue;
        }
      }
      break;
      case EVENT_TYPE_READ: {
        struct sanic_http_request tmp_request = {
          .req_id = uring_req->req_id,
          .conn_fd = uring_req->client_socket
        };
        //TODO: split into read, exec and write
        sanic_handle_connection(uring_req->conn_addr, &tmp_request);
        //EV_SET(change_event, conn_fd, EVFILT_WRITE, EV_ADD, 0, 0, conn_data);

        free(uring_req->iov[0].iov_base);
        free(uring_req);
        GC_gcollect_and_unmap();
      }
      break;
      case EVENT_TYPE_WRITE:
        break;
    }
  }

  return 0;
}

#endif
