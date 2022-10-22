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

  //TODO: add keepalive support
  //TODO: add request validation
  //TODO: add options for auto deserialization

  struct sanic_route **current_route = &routes;
  while (1) {
    if (*current_route == NULL) {
      break;
    }

    if ((*current_route)->parts_count != request->path_len || (*current_route)->method != request->method) {
      goto skip_route;
    }

    for (int i = 0; i < request->path_len; ++i) {
      if((*current_route)->parts[i].type == TYPE_FIXED) {
        if (strcmp((*current_route)->parts[i].value, request->path_parts[i]) != 0) {
          goto skip_route;
        }
      }
    }
    goto route_found;

    skip_route:
    current_route = &(*current_route)->next;
  }
  route_found:

  if (*current_route == NULL) {
    sanic_fmt_log_warn_req(request, "no route for %s %s found", sanic_http_method_to_str(request->method), request->path)
    sanic_finish_request(request, response, addr_str);
    return;
  }

  for (int i = 0; i < request->path_len; ++i) {
    if((*current_route)->parts[i].type == TYPE_PATH_PARAM) {
      struct sanic_http_param *path_param = GC_MALLOC(sizeof(struct sanic_http_param));
      path_param->key = GC_STRDUP((*current_route)->parts[i].value);
      path_param->value = GC_STRDUP(request->path_parts[i]);
      sanic_http_param_insert(&request->path_param, path_param);
    }
  }

  sanic_fmt_log_trace_req(request, "handling request for route %s %s", sanic_http_method_to_str(request->method), request->path)
  response->status = 200;
  (*current_route)->callback(request, response);
  sanic_finish_request(request, response, addr_str);
}
