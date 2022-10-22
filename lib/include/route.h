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
    char* value;
};

struct sanic_route;

struct sanic_route {
    const char *path;

#ifdef SANIC_USE_CLANG_BLOCKS
    void (^callback)(struct sanic_http_request *, struct sanic_http_response *res);
#else
    void (*callback)(struct sanic_http_request *, struct sanic_http_response *res);
#endif

    struct sanic_route *next;
    struct sanic_route_part *parts;
    size_t parts_count;
};

#ifdef DEFINE_ROUTES
struct sanic_route *routes = NULL;
#else
extern struct sanic_route *routes;
#endif

void sanic_insert_route(struct sanic_route route);

void sanic_http_on_get(const char *path,
#ifdef SANIC_USE_CLANG_BLOCKS
        void (^callback)(struct sanic_http_request *, struct sanic_http_response *)
#else
        void (*callback)(struct sanic_http_request *, struct sanic_http_response *)
#endif
);

#endif //SANIC_ROUTE_UTIL_H
