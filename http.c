#define DEFINE_LOG_LEVEL

#include "http.h"
#include "http_request.h"
#include "http_response.h"
#include "log.h"
#include "middleware.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

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

      char *p = malloc(strlen(buf));
      strcpy(p, buf);
      parts = realloc(parts, parts_count * sizeof(char *));
      parts[parts_count - 1] = p;

      bzero(buf, 1000);
    }

    buf[str_count] = route.path[i];
    str_count++;
    i++;
  } while (i <= route_path_len);

  *current = malloc(sizeof(struct sanic_route));
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

void sig_handler(__attribute__((unused)) int signum) {
  sanic_log_info("shutting down server")
  close(sock_fd);
  stop = 1;
  exit(0);
}

void finish_request(struct sanic_http_request *req, struct sanic_http_response *res) {
  FILE *conn_file = fdopen(req->conn_fd, "w+");

  //TODO: handle headers

  if (res->response_body != NULL) {
    fprintf(conn_file, "HTTP/1.1 %d OK\nConnection: Closed\n\n%s", res->status == -1 ? 404 : res->status,
            res->response_body);
  } else {
    fprintf(conn_file, "HTTP/1.1 %d OK\nConnection: Closed\n\n", res->status == -1 ? 404 : res->status);
  }

  fflush(conn_file);
  close(req->conn_fd);

  sanic_destroy_request(req);

  free(res->response_body);
  free(res);
}

int sanic_http_serve(uint16_t port) {
  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    sanic_log_error("socket creation failed")
    return 1;
  }
  sanic_log_trace("socket successfully created")

  struct sockaddr_in sock_addr;
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port = htons(port);

  if (bind(sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0) {
    sanic_log_error("socket bind failed")
    return 1;
  }
  sanic_fmt_log_trace("socket bind to port %d successful", port)

  if (listen(sock_fd, 128) != 0) {
    sanic_fmt_log_error("listen on port %d failed", port)
    return 1;
  }
  sanic_log_info("server listening")

  connection_loop:
  while (!stop) {
    struct sockaddr_in conn_addr;
    size_t len = sizeof(conn_addr);
    int conn_fd = accept(sock_fd, (struct sockaddr *) &conn_addr, (socklen_t *) &len);
    if (conn_fd < 0) {
      sanic_log_warn("server accept failed")
      continue;
    }

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
    sanic_fmt_log_info("serving %s", addr_str)

    struct sanic_http_request *request = sanic_read_request(conn_fd);
    request->conn_fd = conn_fd;

    struct sanic_http_response *response = malloc(sizeof(struct sanic_http_response));
    response->headers = NULL;
    response->response_body = malloc(1);
    bzero(response->response_body, 1);

    sanic_fmt_log_trace("processing middleware for %s", addr_str)

    struct sanic_middleware **current_middleware = &middlewares;
    while (*current_middleware != NULL) {
      enum sanic_middleware_action action = (*current_middleware)->callback(request, response);
      if (action == ACTION_STOP) {
        sanic_fmt_log_warn("stopping request from %s due to middleware response", addr_str)
        finish_request(request, response);
        goto connection_loop;
      }

      current_middleware = &(*current_middleware)->next;
    }

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
      finish_request(request, response);
      continue;
    }

    sanic_fmt_log_trace("handling request for route %s", request->path)
    response->status = 200;
    (*current_route)->callback(request, response);
    finish_request(request, response);
  }

  return 0;
}
