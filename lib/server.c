#define DEFINE_LOG_LEVEL

#include <stdlib.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <uuid4.h>
#include <gc.h>

#include "include/internal/sanic_ascii.h"
#include "include/log.h"
#include "include/internal/request_handler.h"

volatile sig_atomic_t stop;
volatile int sock_fd;

void sanic_shutdown_server() {
  sanic_log_info("shutting down server")

  if (close(sock_fd) != 0) {
    sanic_fmt_log_error("failed to close server socket: %s", strerror(errno))
  }

  stop = 1;
}

void sig_handler(int signum) {
  sanic_log_debug("received interrupt")
  sanic_shutdown_server();
  exit(0);
}

int sanic_http_serve(uint16_t port) {
  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGINT, sig_handler);

#if SANIC_SHOW_LOGO
  printf(sanic_ascii_logo);
#endif

  sanic_log_trace("initializing web server")

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    sanic_fmt_log_error("socket creation failed: %s", strerror(errno))
    return 1;
  }
  sanic_log_debug("socket successfully created")

#if SANIC_SOCKET_NO_LINGER
  struct linger l = {
          .l_onoff = 1,
          .l_linger = 0
  };

  if (setsockopt(sock_fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) != 0) {
    sanic_log_error("setting SO_LINGER failed");
    return 1;
  }
#endif

  struct sockaddr_in sock_addr;
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port = htons(port);

  if (bind(sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0) {
    sanic_fmt_log_error("socket bind failed: %s", strerror(errno))
    return 1;
  }
  sanic_fmt_log_debug("socket bind to port %d successful", port)

  if (listen(sock_fd, 128) != 0) {
    sanic_fmt_log_error("listen on port %d failed: %s", port, strerror(errno))
    return 1;
  }
  sanic_fmt_log_info("server listening on port %d", port)

  while (!stop) {
    struct sockaddr_in conn_addr;
    size_t len = sizeof(conn_addr);

    sanic_log_trace("waiting for new connection")

    int conn_fd = accept(sock_fd, (struct sockaddr *) &conn_addr, (socklen_t *) &len);

    char req_id[UUID4_LEN] = {0};
    bzero(req_id, 37);
    uuid4_generate(req_id);

    struct sanic_http_request tmp_request;
    tmp_request.req_id = req_id;
    tmp_request.conn_fd = conn_fd;

    sanic_log_trace_req(&tmp_request, "accepted new connection")
    if (conn_fd < 0) {
      sanic_log_warn("server accept failed")
      continue;
    }

    sanic_handle_connection(conn_fd, conn_addr, &tmp_request);
  }

  return 0;
}
