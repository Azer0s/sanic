#include "http.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "log.h"
#include "middleware.h"

#ifdef USE_CLANG_BLOCKS

int main() {
  sanic_log_level = LEVEL_TRACE;

  sanic_use_middleware((struct sanic_middleware) {
          .callback = ^enum sanic_middleware_action(struct sanic_http_request *req, struct sanic_http_response *res) {
              if (strcmp(req->path, "/foobar") == 0) {
                return ACTION_STOP;
              }
              return ACTION_PASS;
          }
  });

  sanic_use_middleware((struct sanic_middleware) {
          .callback = ^enum sanic_middleware_action(struct sanic_http_request *req, struct sanic_http_response *res) {
              printf("Hello from my middleware!\n");
              return ACTION_PASS;
          }
  });

  sanic_http_on_get("/", ^void(struct sanic_http_request *req, struct sanic_http_response *res) {
      const char *html = "<h1>Hello, World!</h1>";
      //res->response_body is always pre-allocated with size 1
      res->response_body = realloc(res->response_body, strlen(html));
      strcpy(res->response_body, html);
  });

  sanic_http_on_get("/people/{:name}", ^void(struct sanic_http_request *req, struct sanic_http_response *res) {
      printf("Hello %s!\n", "foo");
  });

  return sanic_http_serve(8080);
}

#else

void handle_index(struct sanic_http_request *req, struct sanic_http_response *res) {
  const char *html = "<h1>Hello, World!</h1>";
  //res->response_body is always pre-allocated with size 1
  res->response_body = realloc(res->response_body, strlen(html));
  strcpy(res->response_body, html);
}

void handle_get_person(struct sanic_http_request *req, struct sanic_http_response *res) {
  printf("Hello %s!\n", "foo");
}

enum sanic_middleware_action foobar_filter(struct sanic_http_request *req, struct sanic_http_response *res) {
  if (strcmp(req->path, "/foobar") == 0) {
    return ACTION_STOP;
  }
  return ACTION_PASS;
}

enum sanic_middleware_action hello_filter(struct sanic_http_request *req, struct sanic_http_response *res) {
  printf("Hello from my middleware!\n");
  return ACTION_PASS;
}

int main() {
  sanic_log_level = LEVEL_TRACE;

  sanic_use_middleware((struct sanic_middleware) { .callback = foobar_filter });
  sanic_use_middleware((struct sanic_middleware) { .callback = hello_filter });

  sanic_http_on_get("/", handle_index);
  sanic_http_on_get("/people/{:name}", handle_get_person);

  return sanic_http_serve(8080);
}


#endif

