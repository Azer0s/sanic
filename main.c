#include "lib/include/http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gc.h>
#include <sanic.h>

enum sanic_middleware_action http_version_filter(struct sanic_http_request *req, struct sanic_http_response *res) {
  char version[4];
  bzero(version, 4);
  strcpy(version, req->version + 5);

  char *err;
  double version_num = strtod(version, &err);

  if (version == err) {
    res->status = 400;
    return ACTION_STOP;
  }

  if (version_num >= 2) {
    res->status = 505;
    return ACTION_STOP;
  }

  return ACTION_PASS;
}

#ifdef USE_CLANG_BLOCKS

int main() {
  GC_init();

  sanic_log_level = LEVEL_DEBUG;

  sanic_use_middleware(^enum sanic_middleware_action(struct sanic_http_request *req, struct sanic_http_response *res) {
    return http_version_filter(req, res);
  });

  sanic_use_middleware(^enum sanic_middleware_action(struct sanic_http_request *req, struct sanic_http_response *res) {
    if (strcmp(req->path, "/teapot") == 0) {
      res->status = 418;
      return ACTION_STOP;
    }
    return ACTION_PASS;
  });

  sanic_http_on_get("/", ^void(struct sanic_http_request *req, struct sanic_http_response *res) {
    const char *html = "<h1>Hello, World!</h1>";
    res->response_body = GC_malloc_atomic(strlen(html));
    strcpy(res->response_body, html);
  });

  sanic_http_on_get("/people/{:name}", ^void(struct sanic_http_request *req, struct sanic_http_response *res) {
    char *name = sanic_get_params_value(req, "name");
    char *html_template = "<h1>Hello, %s!</h1>";
    res->response_body = GC_malloc_atomic(strlen(html_template) + strlen(name) - 2);
    sprintf(res->response_body, html_template, name);
  });

  return sanic_http_serve(8080);
}

#else

void handle_index(struct sanic_http_request *req, struct sanic_http_response *res) {
  const char *html = "<h1>Hello, World!</h1>";
  res->response_body = GC_malloc_atomic(strlen(html));
  strcpy(res->response_body, html);
}

void handle_get_person(struct sanic_http_request *req, struct sanic_http_response *res) {
  char *name = sanic_get_params_value(req, "name");
  char *html_template = "<h1>Hello, %s!</h1>";
  res->response_body = GC_malloc_atomic(strlen(html_template) + strlen(name) - 2);
  sprintf(res->response_body, html_template, name);
}

enum sanic_middleware_action teapot_filter(struct sanic_http_request *req, struct sanic_http_response *res) {
  if (strcmp(req->path, "/teapot") == 0) {
    res->status = 418;
    return ACTION_STOP;
  }
  return ACTION_PASS;
}

int main() {
  sanic_log_level = LEVEL_TRACE;

  sanic_use_middleware(http_version_filter);
  sanic_use_middleware(teapot_filter);

  sanic_http_on_get("/", handle_index);
  sanic_http_on_get("/people/{:name}", handle_get_person);

  return sanic_http_serve(8080);
}

#endif
