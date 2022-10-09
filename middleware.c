#define DEFINE_MIDDLEWARES

#include <string.h>
#include <stdlib.h>
#include "middleware.h"

void sanic_use_middleware(struct sanic_middleware middleware) {
  struct sanic_middleware **current = &middlewares;
  while (*current != NULL) {
    current = &(*current)->next;
  }

  *current = malloc(sizeof(struct sanic_middleware));
  (*current)->callback = middleware.callback;
  (*current)->next = NULL;
}
