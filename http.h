#ifndef SANIC_HTTP_H
#define SANIC_HTTP_H

#include <stdint.h>
#include <stddef.h>
#include "http_request.h"

struct sanic_route;

struct sanic_route {
    const char *path;

#ifdef USE_CLANG_BLOCKS
    void (^callback)(struct sanic_http_request *);
#else
    void (*callback)(struct sanic_http_request *);
#endif

    struct sanic_route *next;
    char **parts;
    size_t parts_count;
};

#ifdef USE_CLANG_BLOCKS
void sanic_http_on_get(const char *route, void (^callback)(struct sanic_http_request *req));
#else
void sanic_http_on_get(const char *route, void (*callback)(struct sanic_http_request *req));
#endif

int sanic_http_serve(uint16_t port);

#endif //SANIC_HTTP_H
