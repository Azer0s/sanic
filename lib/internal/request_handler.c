#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <uv.h>
#include <gc.h>

#include "../include/log.h"
#include "../include/internal/request_handler.h"
#include "../include/route.h"
#include "../include/internal/request_util.h"
#include "../include/internal/http_util.h"

size_t sanic_calculate_response_size(struct sanic_http_response *res) {
  size_t size = 0;
  size += 8; //strlen("HTTP/1.1")
  size += 3; //status code 000-999

  int status = res->status == -1 ? 404 : res->status;
  size += strlen(sanic_get_status_text(status));
  size++; //newline

  bool has_closed_header = false;
  struct sanic_http_param **current = &res->headers;
  while (*current != NULL) {
    if (strcmp((*current)->key, "Connection") == 0) {
      has_closed_header = true;
      size += 10; //strlen("Connection")
    } else {
      size += strlen((*current)->key);
    }

    size += 2; //strlen(": ")
    size += strlen((*current)->value);
    size++; //newline

    current = &(*current)->next;
  }

  if (!has_closed_header) {
    size += 10; //strlen("Connection")
    size += 2; //strlen(": ")
    size += 6; //strlen("Closed")
    size++; //newline
  }

  size++; //newline
  if (res->response_body != NULL) {
    size += strlen(res->response_body);
  }

  return size; //newline
}

void sanic_write_response(char *buff, struct sanic_http_request *req, struct sanic_http_response *res) {
  int status = res->status == -1 ? 404 : res->status;
  int offset = sprintf(buff, "HTTP/1.1 %d %s\n", status, sanic_get_status_text(status));

  struct sanic_http_param closed_header = (struct sanic_http_param) {
    .key = "Connection",
    .value = "Closed"
  };

  sanic_http_param_insert(&res->headers, &closed_header);

  struct sanic_http_param **current = &res->headers;
  while (*current != NULL) {
    offset += sprintf(buff + offset, "%s: %s\n", (*current)->key, (*current)->value);
    current = &(*current)->next;
  }

  if (res->response_body != NULL) {
    sprintf(buff + offset, "\n%s", res->response_body);
  } else {
    sprintf(buff + offset, "\n");
  }
}

/*void sanic_finish_request(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str) {
  if (req->conn_file == NULL) {
    sanic_log_warn_req(req, "file is not open");

    if ((req->conn_file = fdopen(req->conn_fd, "w+")) == NULL) {
      sanic_fmt_log_warn_req(req, "could not open file: %s", strerror(errno));
      return;
    }
  }

  int status = res->status == -1 ? 404 : res->status;
  fprintf(req->conn_file, "HTTP/1.1 %d %s\n", status, sanic_get_status_text(status));

  struct sanic_http_param closed_header = (struct sanic_http_param) {
    .key = "Connection",
    .value = "Closed"
  };

  sanic_http_param_insert(&res->headers, &closed_header);

  struct sanic_http_param **current = &res->headers;
  while (*current != NULL) {
    fprintf(req->conn_file, "%s: %s\n", (*current)->key, (*current)->value);
    current = &(*current)->next;
  }

  if (res->response_body != NULL) {
    fprintf(req->conn_file, "\n%s", res->response_body);
  } else {
    fprintf(req->conn_file, "\n");
  }

  if (fclose(req->conn_file) != 0) {
    sanic_fmt_log_warn_req(req, "could not close file: %s", strerror(errno));
  }

  if (fcntl(req->conn_fd, F_GETFD) == 0) {
    if (close(req->conn_fd) == 0) {
      sanic_fmt_log_debug_req(req, "closed connection to %s", addr_str);
    } else {
      sanic_fmt_log_warn("failed to close connection to %s: %s", addr_str, strerror(errno));
    }
  }

  GC_FREE(req);
  GC_FREE(res);
}*/

struct sanic_proceed_or_reply sanic_handle_connection_read(const uv_buf_t *buff, struct sanic_http_request *init_req, char *addr_str) {
  struct sanic_proceed_or_reply ret;
  ret.reply = false;

  sanic_fmt_log_info_req(init_req, "serving %s", addr_str);

  //TODO: add request validation
  //TODO: add options for auto deserialization
  struct sanic_http_request *request = sanic_read_request(buff, init_req);
  ret.req = request;

  if (request == NULL) {
    sanic_log_warn_req(init_req, "there was an error reading the request");
    ret.reply = true;
    ret.res = &(struct sanic_http_response) {
      .status = 400,
    };
  }

  struct sanic_http_response *response = GC_MALLOC_UNCOLLECTABLE(sizeof(struct sanic_http_response));
  response->headers = NULL;
  response->status = -1;
  ret.res = response;

  return ret;
}

void sanic_handle_connection_make_response(struct sanic_http_request *request, struct sanic_http_response *response) {
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
    sanic_fmt_log_warn_req(request, "no route for %s %s found", sanic_http_method_to_str(request->method), request->path);
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

  sanic_fmt_log_trace_req(request, "handling request for route %s %s", sanic_http_method_to_str(request->method), request->path);
  response->status = 200;
  (*current_route)->callback(request, response);
}
