#include <string.h>
#include <gc.h>
#include "include/http_header.h"

void sanic_http_header_insert(struct sanic_http_header **headers, struct sanic_http_header *header) {
  struct sanic_http_header **current = headers;
  while (*current != NULL) {
    if (strcmp((*current)->key, header->key) == 0) {
      (*current)->value = header->value;
      return;
    }

    current = &(*current)->next;
  }

  *current = header;
}
