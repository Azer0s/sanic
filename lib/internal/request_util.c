#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include "gc.h"

#include "../include/internal/request_util.h"
#include "../include/http_method.h"
#include "../include/http_request.h"

void parse_request_meta(struct sanic_http_request *request, char *tmp, ssize_t n) {
  int i = 0;
  while (tmp[i] != ' ') {
    ++i;
  }
  char *method = GC_MALLOC_ATOMIC(i);
  strncpy(method, tmp, i);

  for (int j = 0; method[j]; j++) {
    method[j] = toupper(method[j]); // NOLINT(cppcoreguidelines-narrowing-conversions)
  }

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
  char *path = GC_malloc_atomic((i - from) + 1);
  bzero(path, (i + from) + 1);
  strncpy(path, tmp + from, i - from);

  request->path = path;

  if (i <= n) {
    i++;
    ssize_t to = n - 2;
    char *version = GC_malloc_atomic((to - i) + 1);
    bzero(version, (to - i) + 1);
    strncpy(version, tmp + i, to - i);

    request->version = version;
  }

  GC_free(method);
}

void parse_request_header(struct sanic_http_request *request, char *tmp, ssize_t n) {
  //TODO: check header for format validity

  int i = 0;
  while (tmp[i] != ' ') {
    ++i;
  }
  i--;
  char *key = GC_MALLOC_ATOMIC(i + 1);
  bzero(key, i + 1);
  strncpy(key, tmp, i);

  i += 2; //skip ': '

  ssize_t to = n - 2;
  char *value = GC_MALLOC_ATOMIC((to - i) + 1);
  bzero(value, (to - i) + 1);
  strncpy(value, tmp + i, to - i);

  struct sanic_http_header *new = GC_MALLOC(sizeof(struct sanic_http_header));
  new->key = key;
  new->value = value;
  new->next = NULL;

  sanic_http_header_insert(&request->headers, new);
}

struct sanic_http_request *sanic_read_request(int fd) {
  //TODO: Add error handling for invalid HTTP requests

  FILE *conn_file;

  if ((conn_file = fdopen(fd, "r")) == NULL) {
    //TODO: report error
    return NULL;
  }

  size_t size = 1;
  char *block = GC_MALLOC_ATOMIC(sizeof(char) * size);
  *block = '\0';

  char *tmp = malloc(sizeof(char) * size);
  *tmp = '\0';

  size_t old_size = 1;

  enum {
      PATH,
      HEADER,
  } mode = PATH;

  struct sanic_http_request *request = GC_MALLOC(sizeof(struct sanic_http_request));
  request->headers = NULL;

  ssize_t n;
  while ((n = getline(&tmp, &size, conn_file)) > 0) {
    if (strcmp(tmp, "\r\n") == 0) {
      break;
    }
    block = GC_REALLOC(block, size + old_size);
    old_size += size;
    strcat(block, tmp);

    switch (mode) {
      case PATH:
        parse_request_meta(request, tmp, n);
        mode = HEADER;
        break;

      case HEADER:
        parse_request_header(request, tmp, n);
        break;

      default:
        assert(false);
    }
  }

  free(tmp);
  return request;
}
