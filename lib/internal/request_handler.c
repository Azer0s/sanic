#include <stdio.h>
#include <gc.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>

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

  struct sanic_http_param closed_header = (struct sanic_http_param) {
    .key = "Connection",
    .value = "Closed"
  };

  sanic_http_param_insert(&res->headers, &closed_header);

  struct sanic_http_param **current = &res->headers;
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

  GC_FREE(req);
  GC_FREE(res);
}

struct connection_thread_data {
    int conn_fd;
    struct sockaddr_in conn_addr;
    struct sanic_http_request *init_req;
};

void sanic_connection_thread(int conn_fd, struct sockaddr_in conn_addr, struct sanic_http_request *init_req) {
  char addr_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
  sanic_fmt_log_info_req(init_req, "serving %s", addr_str)

  struct sanic_http_request *request = sanic_read_request(conn_fd, init_req);
  if (request == NULL) {
    sanic_log_warn_req(init_req, "there was an error reading the request")
    sanic_finish_request(init_req, &(struct sanic_http_response) {
      .status = 400,
    }, addr_str);
    return;
  }

  struct sanic_http_response *response = GC_MALLOC(sizeof(struct sanic_http_response));
  response->headers = NULL;
  response->status = -1;

  if (sanic_handle_middlewares(request, response, addr_str) == ACTION_STOP) {
    return;
  }

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
      if ((*current_route)->parts[i].type == TYPE_FIXED) {
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
    if ((*current_route)->parts[i].type == TYPE_PATH_PARAM) {
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

void *connection_thread_bootstrap(void *thread_data) {
  struct connection_thread_data *thread_data_struct = thread_data;
  sanic_connection_thread(thread_data_struct->conn_fd, thread_data_struct->conn_addr, thread_data_struct->init_req);
  free(thread_data);

  GC_gcollect_and_unmap();

#if GC_DEBUG
  GC_dump();

  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
#endif

  return NULL;
}

void sanic_handle_connection(int conn_fd, struct sockaddr_in conn_addr, struct sanic_http_request *init_req) {
  struct connection_thread_data *thread_data = malloc(sizeof(struct connection_thread_data));
  thread_data->conn_fd = conn_fd;
  thread_data->conn_addr = conn_addr;
  thread_data->init_req = init_req;

  //TODO: add ring buffer for threads
  //if the buffer is full, join the first inserted pthread
  //and remove it from the ringbuffer
  //then insert the new thread

  pthread_t conn_thread;
  GC_pthread_create(&conn_thread, NULL, connection_thread_bootstrap, thread_data);
  GC_pthread_join(conn_thread, NULL);
}
