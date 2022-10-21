#define DEFINE_MIDDLEWARES

#include <string.h>
#include "include/middleware.h"
#include "include/http_response.h"
#include "include/http_request.h"
#include <gc.h>

void sanic_use_middleware(
#ifdef USE_CLANG_BLOCKS
        enum sanic_middleware_action (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        enum sanic_middleware_action (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
) {
  struct sanic_middleware **current = &middlewares;
  while (*current != NULL) {
    current = &(*current)->next;
  }

  *current = GC_MALLOC(sizeof(struct sanic_middleware));
  (*current)->callback = callback;
  (*current)->next = NULL;
}
