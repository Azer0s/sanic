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
  (*current)->method = route.method;
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
  struct sanic_route route = {
    .path = path,
    .callback = callback,
    .method = method
  };
  sanic_insert_route(route);
}

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)

#ifdef SANIC_USE_CLANG_BLOCKS
#define sanic_route_impl(which, method) void GLUE(sanic_http_on_, which)( \
const char *path, \
void (^callback)(struct sanic_http_request *, struct sanic_http_response *) \
) { \
  sanic_http_on(method, path, callback); \
}
#else
#define sanic_route_impl(which, method) void GLUE(sanic_http_on_, which)( \
const char *path, \
void (*callback)(struct sanic_http_request *, struct sanic_http_response *) \
) { \
  sanic_http_on(method, path, callback); \
}
#endif

sanic_route_impl(get, METHOD_GET)
sanic_route_impl(head, METHOD_HEAD)
sanic_route_impl(post, METHOD_POST)
sanic_route_impl(put, METHOD_PUT)
sanic_route_impl(delete, METHOD_DELETE)
sanic_route_impl(connect, METHOD_CONNECT)
sanic_route_impl(options, METHOD_OPTIONS)
sanic_route_impl(trace, METHOD_TRACE)
sanic_route_impl(patch, METHOD_PATCH)
