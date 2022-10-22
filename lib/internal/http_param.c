#include <string.h>
#include "../include/internal/http_param.h"

void sanic_http_param_insert(struct sanic_http_param **params, struct sanic_http_param *param) {
  struct sanic_http_param **current = params;
  while (*current != NULL) {
    if (strcmp((*current)->key, param->key) == 0) {
      (*current)->value = param->value;
      return;
    }

    current = &(*current)->next;
  }

  *current = param;
}

struct sanic_http_param *sanic_http_param_get(struct sanic_http_param **params, char *key) {
  struct sanic_http_param **current = params;
  while (*current != NULL) {
    if (strcmp((*current)->key, key) == 0) {
      return *current;
    }
    current = &(*current)->next;
  }
  return NULL;
}