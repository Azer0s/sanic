#define DEFINE_LOG_LEVEL

#include "include/http.h"
#include "include/http_request.h"
#include "include/http_response.h"
#include "include/log.h"
#include "include/middleware.h"
#include <stdlib.h>
#include "../ext/bdwgc/include/gc/gc.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <uuid/uuid.h>
#include "include/sanic_ascii.h"

enum sanic_log_level_enum sanic_log_level;

struct sanic_route *routes = NULL;

void insert_route(struct sanic_route route) {
  struct sanic_route **current = &routes;
  while (*current != NULL) {
    current = &(*current)->next;
  }

  //TODO: parse route parts and param filter

  size_t parts_count = 0;
  char **parts = NULL;

  char buf[1000];
  bzero(buf, 1000);
  int str_count = 0;

  size_t route_path_len = strlen(route.path);
  int i = 0;
  do {
    if ((route.path[i] == '/' && i != 0) || i == route_path_len) {
      str_count = 0;
      parts_count++;

      char *p = GC_MALLOC_ATOMIC(strlen(buf));
      strcpy(p, buf);
      parts = GC_REALLOC(parts, parts_count * sizeof(char *));
      parts[parts_count - 1] = p;

      bzero(buf, 1000);
    }

    buf[str_count] = route.path[i];
    str_count++;
    i++;
  } while (i <= route_path_len);

  *current = GC_MALLOC(sizeof(struct sanic_route));
  (*current)->path = route.path;
  (*current)->callback = route.callback;
  (*current)->next = NULL;
  (*current)->parts = parts;
  (*current)->parts_count = parts_count;
}

void sanic_http_on_get(const char *path,
#ifdef USE_CLANG_BLOCKS
        void (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        void (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
) {
  struct sanic_route route;
  route.path = path;
  route.callback = callback;
  insert_route(route);
}

volatile sig_atomic_t stop;
volatile int sock_fd;

void sanic_shutdown_server() {
  sanic_log_info("shutting down server")

  if (close(sock_fd) != 0) {
    sanic_fmt_log_error("failed to close server socket: %s", strerror(errno))
  }

  stop = 1;
}

void sig_handler(__attribute__((unused)) int signum) {
  sanic_log_debug("received interrupt")
  sanic_shutdown_server();
  exit(0);
}

void finish_request(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str) {
  FILE *conn_file = fdopen(req->conn_fd, "w+");

  //TODO: handle headers
  int status = res->status == -1 ? 404 : res->status;

  if (res->response_body != NULL) {
    fprintf(conn_file, "HTTP/1.1 %d %s\nConnection: Closed\n\n%s", status, sanic_get_status_text(status),
            res->response_body);
  } else {
    fprintf(conn_file, "HTTP/1.1 %d %s\nConnection: Closed\n\n", status, sanic_get_status_text(status));
  }

  fflush(conn_file);

  if (close(req->conn_fd) == 0) {
    sanic_fmt_log_debug_req(req, "closed connection to %s", addr_str)
  } else {
    sanic_fmt_log_warn("failed to close connection to %s", addr_str)
  }

  GC_gcollect();

#if GC_DEBUG
  printf("%zu\n", GC_get_heap_size());
  printf("%zu\n", GC_get_free_bytes());
#endif
}

int sanic_http_serve(uint16_t port) {
  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGINT, sig_handler);

  printf(sanic_ascii_logo);

  sanic_log_trace("initializing web server")

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    sanic_fmt_log_error("socket creation failed: %s", strerror(errno))
    return 1;
  }
  sanic_log_debug("socket successfully created")

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

  connection_loop:
  while (!stop) {
    struct sockaddr_in conn_addr;
    size_t len = sizeof(conn_addr);

    sanic_log_trace("waiting for new connection")

    int conn_fd = accept(sock_fd, (struct sockaddr *) &conn_addr, (socklen_t *) &len);

    char req_id[37] = {0};
    bzero(req_id, 37);
    uuid_t req_id_struct;
    uuid_generate(req_id_struct);
    uuid_unparse_lower(req_id_struct, req_id);

    struct sanic_http_request tmp_request;
    tmp_request.req_id = req_id;
    tmp_request.conn_fd = conn_fd;

    sanic_log_trace_req(&tmp_request, "accepted new connection")
    if (conn_fd < 0) {
      sanic_log_warn("server accept failed")
      continue;
    }

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
    sanic_fmt_log_info_req(&tmp_request, "serving %s", addr_str)

    struct sanic_http_request *request = sanic_read_request(conn_fd);
    request->conn_fd = conn_fd;
    request->req_id = req_id;

    struct sanic_http_response *response = GC_MALLOC(sizeof(struct sanic_http_response));
    response->headers = NULL;
    response->status = -1;

    sanic_fmt_log_trace_req(request, "processing middleware for %s", addr_str)

    struct sanic_middleware **current_middleware = &middlewares;
    while (*current_middleware != NULL) {
      enum sanic_middleware_action action = (*current_middleware)->callback(request, response);
      if (action == ACTION_STOP) {
        sanic_fmt_log_warn("stopping request from %s due to middleware response", addr_str)
        finish_request(request, response, addr_str);
        goto connection_loop;
      }

      current_middleware = &(*current_middleware)->next;
    }

    //TODO: handle path params
    //TODO: handle query params
    //TODO: add request validation
    //TODO: add options for auto deserialization

    struct sanic_route **current_route = &routes;
    while (1) {
      if (*current_route == NULL) {
        break;
      }

      if (strcmp(request->path, (*current_route)->path) == 0) {
        break;
      }

      current_route = &(*current_route)->next;
    }

    if (*current_route == NULL) {
      sanic_fmt_log_warn("no route %s found", request->path)
      finish_request(request, response, addr_str);
      continue;
    }

    sanic_fmt_log_trace_req(request, "handling request for route %s", request->path)
    response->status = 200;
    (*current_route)->callback(request, response);
    finish_request(request, response, addr_str);
  }

  return 0;
}
