#include "http.h"
#include <stdio.h>
#include "log.h"

int main(int argc, char *argv[]) {
  // sanic_log_level = LEVEL_INFO;

  sanic_http_on_get("/", ^void(struct sanic_http_request *req) {
    printf("Hello!\n");
  });

  return sanic_http_serve(8080);
}
