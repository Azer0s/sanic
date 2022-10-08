#include "http.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  sanic_http_on_get("/", ^void(struct sanic_http_request *req) {
      printf("Hello!");
  });

  return sanic_http_serve(8080);
}
