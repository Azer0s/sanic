#define DEFINE_INTERNAL_VARS

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <liburing.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <gc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <errno.h>
#include <uuid4.h>
#include <gc.h>

#include "../include/internal/sanic_ascii.h"
#include "../include/log.h"
#include "../include/internal/server_internals.h"
#include "../include/internal/string_util.h"

void (*stop_callback)(void);
char *(*sig2str_callback)(int);

void shutdown_server() {
  sanic_log_info("shutting down server");

  if (stop_callback != NULL) {
    stop_callback();
  }

  if (close(sock_fd) != 0) {
    sanic_fmt_log_error("failed to close server socket: %s", strerror(errno));
  }

  stop = 1;
}

void sig_handler(int signum) {
  char *sig_name = GC_STRDUP(sig2str_callback(signum));
  sanic_fmt_log_debug("received interrupt: SIG%s", str_uppercase(sig_name, strlen(sig_name)));
  shutdown_server();
  exit(0);
}

void sanic_setup_interrupts(void (*stop_cb)(void), char *(*sig2str_cb)(int)) {
  stop_callback = stop_cb;
  sig2str_callback = sig2str_cb;

  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGINT, sig_handler);
}

int sanic_create_socket(uint16_t port) {
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

  if (bind(sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0) {
    sanic_fmt_log_error("socket bind failed: %s", strerror(errno));
    return 1;
  }
  sanic_fmt_log_debug("socket bind to port %d successful", port);

  if (listen(sock_fd, 128) != 0) {
    sanic_fmt_log_error("listen on port %d failed: %s", port, strerror(errno));
    return 1;
  }
  sanic_fmt_log_info("server listening on port %d", port);

  return 0;
}

