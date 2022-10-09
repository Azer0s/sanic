#include "http.h"
#include <stdio.h>
#include "log.h"

void handle_index(struct sanic_http_request* req) {
  printf("Hello! \n");
}

void handle_get_person(struct sanic_http_request* req) {
  printf("Hello %s!\n", "foo");
}

int main(int argc, char *argv[]) {
  sanic_log_level = LEVEL_TRACE;

#ifdef USE_CLANG_BLOCKS
  sanic_http_on_get("/", ^void(struct sanic_http_request *req) {
    printf("Hello!\n");
  });

  sanic_http_on_get("/people/{:name}", ^void(struct sanic_http_request *req) {
    printf("Hello %s!\n", "foo");
  });
#else
  sanic_http_on_get("/", handle_index);
  sanic_http_on_get("/people/{:name}", handle_get_person);
#endif

  return sanic_http_serve(8080);
}
