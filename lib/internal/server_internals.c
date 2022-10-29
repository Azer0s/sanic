#define DEFINE_INTERNAL_VARS

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <gc.h>
#include <errno.h>

#include "../include/log.h"
#include "../include/internal/server_internals.h"
#include "../include/internal/string_util.h"

void shutdown_server() {
  sanic_log_info("shutting down server");
  sanic_stop_serve_internal();

  if (close(sock_fd) != 0) {
    sanic_fmt_log_error("failed to close server socket: %s", strerror(errno));
  }

  stop = 1;
}

void sig_handler(int signum) {
#if __APPLE__
  char *sig_name = GC_STRDUP(sys_signame[signum]);
#elif __linux__
  char *sig_name = GC_STRDUP(strsignal(signum));
#endif

  sanic_fmt_log_debug("received interrupt: SIG%s", str_uppercase(sig_name, strlen(sig_name)));
  shutdown_server();
  exit(0);
}

void sanic_setup_interrupts() {
  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGINT, sig_handler);
}

