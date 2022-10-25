#ifndef SANIC_ROUTE_UTIL_H
#define SANIC_ROUTE_UTIL_H

#include "http_request.h"
#include "http_response.h"

enum sanic_route_part_type {
    TYPE_FIXED,
    TYPE_PATH_PARAM
};

struct sanic_route_part {
    enum sanic_route_part_type type;
    char *value;
};

struct sanic_route;

struct sanic_route {
    const char *path;

#ifdef SANIC_USE_CLANG_BLOCKS
    void (^callback)(struct sanic_http_request *, struct sanic_http_response *res);
#else
    void (*callback)(struct sanic_http_request *, struct sanic_http_response *res);
#endif

    struct sanic_route_part *parts;
    size_t parts_count;
    enum sanic_http_method method;
    struct sanic_route *next;
};

#ifdef DEFINE_ROUTES
struct sanic_route *routes = NULL;
#else
extern struct sanic_route *routes;
#endif

void sanic_insert_route(struct sanic_route route);

//TODO: add other methods

void sanic_http_on(enum sanic_http_method method, const char *path,
#ifdef SANIC_USE_CLANG_BLOCKS
        void (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        void (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
);

#define GLUE_HELPER(x, y) x##y
#define GLUE(x, y) GLUE_HELPER(x, y)

#ifdef SANIC_USE_CLANG_BLOCKS
#define sanic_route_def(which, method) void GLUE(sanic_http_on_, which)( \
const char *path, \
void (^callback)(struct sanic_http_request *, struct sanic_http_response *) \
);
#else
#define sanic_route_def(which, method) void GLUE(sanic_http_on_, which)( \
const char *path, \
void (*callback)(struct sanic_http_request *, struct sanic_http_response *) \
);
#endif

sanic_route_def(get, METHOD_GET)
sanic_route_def(head, METHOD_HEAD)
sanic_route_def(post, METHOD_POST)
sanic_route_def(put, METHOD_PUT)
sanic_route_def(delete, METHOD_DELETE)
sanic_route_def(connect, METHOD_CONNECT)
sanic_route_def(options, METHOD_OPTIONS)
sanic_route_def(trace, METHOD_TRACE)
sanic_route_def(patch, METHOD_PATCH)

#endif //SANIC_ROUTE_UTIL_H
