#if SANIC_IO_URING

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <liburing.h>
#include <string.h>

#include "include/log.h"
#include "include/internal/server_internals.h"

struct io_uring ring;

#define EVENT_TYPE_ACCEPT       0
#define EVENT_TYPE_READ         1
#define EVENT_TYPE_WRITE        2

struct request {
  int event_type;
  int iovec_count;
  int client_socket;
  struct iovec iov[];
};

int add_accept_request(int server_socket, struct sockaddr_in *client_addr, socklen_t *client_addr_len) {
  struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  io_uring_prep_accept(sqe, server_socket, (struct sockaddr *) client_addr,
                       client_addr_len, 0);
  struct request *req = malloc(sizeof(struct request));
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
  struct io_uring_cqe *cqe;

  add_accept_request(sock_fd, &conn_addr, &len);

  while (!stop) {

  }

  return 0;
}

#endif
