#include <stdio.h>
#include "gc.h"
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "../include/log.h"
#include "../include/internal/request_handler.h"
#include "../include/middleware.h"
#include "../include/route.h"
#include "../include/internal/request_util.h"
#include "../include/internal/http_util.h"
#include "../include/internal/middleware_handler.h"

void sanic_finish_request(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str) {
  FILE *conn_file = fdopen(req->conn_fd, "w+");

  int status = res->status == -1 ? 404 : res->status;
  fprintf(conn_file, "HTTP/1.1 %d %s\n", status, sanic_get_status_text(status));

  struct sanic_http_header closed_header = (struct sanic_http_header) {
          .key = "Connection",
          .value = "Closed"
  };

  sanic_http_header_insert(&res->headers, &closed_header);

  struct sanic_http_header **current = &res->headers;
  while (*current != NULL) {
    fprintf(conn_file, "%s: %s\n", (*current)->key, (*current)->value);
    current = &(*current)->next;
  }

  if (res->response_body != NULL) {
    fprintf(conn_file, "\n%s", res->response_body);
  } else {
    fprintf(conn_file, "\n");
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

void sanic_handle_connection(int conn_fd, struct sockaddr_in conn_addr, struct sanic_http_request *init_req) {
  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
  sanic_fmt_log_info_req(init_req, "serving %s", addr_str)

  struct sanic_http_request *request = sanic_read_request(conn_fd);
  request->conn_fd = conn_fd;
  request->req_id = init_req->req_id;

  struct sanic_http_response *response = GC_MALLOC(sizeof(struct sanic_http_response));
  response->headers = NULL;
  response->status = -1;

  if (sanic_handle_middlewares(request, response, addr_str) == ACTION_STOP) {
    return;
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

    //TODO: match with route parts
    if (strcmp(request->path, (*current_route)->path) == 0) {
      break;
    }

    current_route = &(*current_route)->next;
  }

  if (*current_route == NULL) {
    sanic_fmt_log_warn_req(request, "no route for %s found", request->path)
    sanic_finish_request(request, response, addr_str);
    return;
  }

  sanic_fmt_log_trace_req(request, "handling request for route %s", request->path)
  response->status = 200;
  (*current_route)->callback(request, response);
  sanic_finish_request(request, response, addr_str);
}
