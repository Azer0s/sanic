#define DEFINE_MIDDLEWARES

#include <stdlib.h>

#include "include/middleware.h"
#include "include/http_response.h"
#include "include/http_request.h"

void sanic_use_middleware(
#ifdef SANIC_USE_CLANG_BLOCKS
        enum sanic_middleware_action (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        enum sanic_middleware_action (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
) {
  struct sanic_middleware **current = &middlewares;
  while (*current != NULL) {
    current = &(*current)->next;
  }

  //We malloc this because the GC should not scan this and this data stays with us
  *current = malloc(sizeof(struct sanic_middleware));
  (*current)->callback = callback;
  (*current)->next = NULL;
}
