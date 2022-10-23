#define DEFINE_ROUTES

#include <string.h>
#include <stdlib.h>

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

      size_t buf_len = strlen(buf);

      if (strncmp(buf, "/{:", 3) == 0 && buf[buf_len - 1] == '}') {
        p = strndup(buf + 3, buf_len - 4);
        type = TYPE_PATH_PARAM;
      } else {
        p = strdup(buf + 1);
        type = TYPE_FIXED;
      }

      parts = realloc(parts, parts_count * sizeof(struct sanic_route_part));
      parts[parts_count - 1].type = type;
      parts[parts_count - 1].value = p;

      bzero(buf, 1000);
    }

    buf[str_count] = route.path[i];
    str_count++;
    i++;
  } while (i <= route_path_len);

  //We malloc this because the GC should not scan this and this data stays with us
  *current = malloc(sizeof(struct sanic_route));
  (*current)->path = route.path;
  (*current)->callback = route.callback;
  (*current)->next = NULL;
  (*current)->parts = parts;
  (*current)->parts_count = parts_count;
}

void sanic_http_on(enum sanic_http_method method, const char *path,
#ifdef SANIC_USE_CLANG_BLOCKS
        void (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        void (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
) {
  struct sanic_route route;
  route.path = path;
  route.callback = callback;
  route.method = method;
  sanic_insert_route(route);
}

void sanic_http_on_get(const char *path,
#ifdef SANIC_USE_CLANG_BLOCKS
        void (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        void (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
) {
  sanic_http_on(METHOD_GET, path, callback);
}

