#include <string.h>
#include "include/http_param.h"

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