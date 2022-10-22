#define DEFINE_ROUTES

#include <string.h>
#include <gc.h>

#include "include/route.h"

void sanic_insert_route(struct sanic_route route) {
  struct sanic_route **current = &routes;
  while (*current != NULL) {
    current = &(*current)->next;
  }

  size_t parts_count = 0;
  struct sanic_route_part *parts = NULL;

  char buf[1000];
  bzero(buf, 1000);
  int str_count = 0;

  size_t route_path_len = strlen(route.path);
  int i = 0;
  do {
    if ((route.path[i] == '/' && i != 0) || i == route_path_len) {
      str_count = 0;
      parts_count++;

      char *p;
      enum sanic_route_part_type type;

      if (strncmp(buf, "/{:", 3) == 0 && buf[strlen(buf) - 1] == '}') {
        p = GC_MALLOC_ATOMIC(strlen(buf) - 3);
        bzero(p, strlen(buf) - 3);
        strncpy(p, buf + 3, strlen(buf) - 4);
        type = TYPE_PATH_PARAM;
      } else {
        p = GC_MALLOC_ATOMIC(strlen(buf));
        strcpy(p, buf);
        type = TYPE_FIXED;
      }

      parts = GC_REALLOC(parts, parts_count * sizeof(struct sanic_route_part));
      parts[parts_count - 1].type = type;
      parts[parts_count - 1].value = p;

      bzero(buf, 1000);
    }

    buf[str_count] = route.path[i];
    str_count++;
    i++;
  } while (i <= route_path_len);

  *current = GC_MALLOC(sizeof(struct sanic_route));
  (*current)->path = route.path;
  (*current)->callback = route.callback;
  (*current)->next = NULL;
  (*current)->parts = parts;
  (*current)->parts_count = parts_count;
}

void sanic_http_on_get(const char *path,
#ifdef SANIC_USE_CLANG_BLOCKS
        void (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        void (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
) {
  struct sanic_route route;
  route.path = path;
  route.callback = callback;
  sanic_insert_route(route);
}

