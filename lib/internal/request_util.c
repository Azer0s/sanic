#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <gc.h>
#include <errno.h>

#include "../include/internal/request_util.h"
#include "../include/http_method.h"
#include "../include/http_request.h"
#include "../include/log.h"
#include "../include/internal/string_util.h"

void parse_request_meta(struct sanic_http_request *request, char *tmp, size_t n) {
  int i = 0;
  while (tmp[i] != ' ') {
    ++i;
  }
  char *method = GC_STRNDUP(tmp, i);
  str_uppercase(method, i);

  if (strcmp(method, "GET") == 0) {
    request->method = METHOD_GET;
  } else if (strcmp(method, "HEAD") == 0) {
    request->method = METHOD_HEAD;
  } else if (strcmp(method, "POST") == 0) {
    request->method = METHOD_POST;
  } else if (strcmp(method, "PUT") == 0) {
    request->method = METHOD_PUT;
  } else if (strcmp(method, "DELETE") == 0) {
    request->method = METHOD_DELETE;
  } else if (strcmp(method, "CONNECT") == 0) {
    request->method = METHOD_CONNECT;
  } else if (strcmp(method, "OPTIONS") == 0) {
    request->method = METHOD_OPTIONS;
  } else if (strcmp(method, "TRACE") == 0) {
    request->method = METHOD_TRACE;
  } else if (strcmp(method, "PATCH") == 0) {
    request->method = METHOD_PATCH;
  }

  int from = ++i;
  while (tmp[i] != ' ') {
    ++i;
  }
  char *path = GC_STRNDUP(tmp + from, i - from);
  request->path = path;

  char *request_path = NULL;
  char *request_query = NULL;

  //Lookup if there is a query part
  size_t request_path_length = 0;
  size_t old_request_path_length = strlen(request->path);
  while (request_path_length < old_request_path_length && request->path[request_path_length] != '?') {
    ++request_path_length;
  }

  //If there is no query part, continue as is
  if (request_path_length != old_request_path_length) {
    //Allocate memory for the query part
    request_path = GC_STRDUP(request->path);

    //Allocate temporary memory for the query part
    size_t request_query_length = old_request_path_length - request_path_length;
    request_query = GC_STRNDUP(request->path + request_path_length + 1, request_query_length);

    char *query_token;
    char *query_rest = request_query;

    //Search for query params
    while ((query_token = strtok_r(query_rest, "&", &query_rest))) {
      char *k = strtok(query_token, "=");
      if (k == NULL) {
        continue;
      }

      char *v = strtok(NULL, "=");
      if (v == NULL) {
        continue;
      }

      //Allocate memory for the query param and insert it into the request
      struct sanic_http_param *param = GC_MALLOC(sizeof(struct sanic_http_param));
      param->key = GC_STRDUP(k);
      param->value = GC_STRDUP(v);
      //TODO: decode query param

      sanic_http_param_insert(&request->query_param, param);
    }
    GC_FREE(request->path);
    GC_FREE(request_query);

    request->path = request_path;
  }

  if (request_path == NULL) {
    request_path = request->path;
  }

  request->path_parts = NULL;
  char *path_token;
  char *path_rest = GC_STRDUP(request_path);
  while ((path_token = strtok_r(path_rest, "/", &path_rest))) {
    request->path_len++;
    request->path_parts = GC_REALLOC(request->path_parts, request->path_len * sizeof(char *));
    request->path_parts[request->path_len - 1] = GC_STRDUP(path_token);
    //TODO: decode path param
  }

  if (request->path_len == 0) {
    request->path_len = 1;
    request->path_parts = GC_MALLOC(request->path_len * sizeof(char *));
    request->path_parts[0] = GC_STRDUP("");
  }

  if (i <= n) {
    i++;
    size_t to = n - 1;
    char *version = GC_STRNDUP(tmp + i, to - i);
    request->version = version;
  }

  GC_FREE(method);
}

void parse_request_header(struct sanic_http_request *request, char *tmp, size_t n) {
  //TODO: check header for format validity

  int i = 0;
  while (tmp[i] != ' ') {
    ++i;
  }
  i--;
  char *key = GC_STRNDUP(tmp, i);

  i += 2; //skip ': '

  size_t to = n - 1;
  char *value = GC_STRNDUP(tmp + i, to - i);

  struct sanic_http_param *new = GC_MALLOC(sizeof(struct sanic_http_param));
  new->key = key;
  new->value = value;
  new->next = NULL;

  sanic_http_param_insert(&request->headers, new);
}

struct sanic_http_request *sanic_read_request(const uv_buf_t *buff, struct sanic_http_request *init_req) {
  //TODO: Add error handling for invalid HTTP requests
  //TODO: Add cookie support
  enum {
      PATH,
      HEADER,
      BODY
  } mode = PATH;

  struct sanic_http_request *request = GC_MALLOC_UNCOLLECTABLE(sizeof(struct sanic_http_request));
  request->conn_fd = init_req->conn_fd;
  request->req_id = init_req->req_id;
  request->headers = NULL;

  char *line;
  char *rest = buff->base;

  //Search for query params
  while ((line = strtok_r(rest, "\n", &rest))) {
    if (strcmp(line, "\r") == 0) {
      break;
    }

    switch (mode) {
      case PATH:
        parse_request_meta(request, line, strlen(line));
        mode = HEADER;
        break;

      case HEADER:
        parse_request_header(request, line, strlen(line));
        break;

      default:
        assert(false);
    }
  }

  struct sanic_http_param *content_length;
  if ((content_length = sanic_http_param_get(&request->headers, "Content-Length")) != NULL) {
    char *lenEnd;
    int len = (int) strtol(content_length->value, &lenEnd, 10);

    if (content_length->value != lenEnd) {
      request->body = GC_STRNDUP(rest, len);;
    }
  }

  if (request->path == NULL || request->version == NULL) {
    return NULL;
  }

  return request;
}
