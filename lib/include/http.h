#ifndef SANIC_HTTP_H
#define SANIC_HTTP_H

#include <stdint.h>
#include <stddef.h>
#include "http_request.h"
#include "http_response.h"

struct sanic_route;

struct sanic_route {
    const char *path;

#ifdef USE_CLANG_BLOCKS
    void (^callback)(struct sanic_http_request *, struct sanic_http_response *res);
#else
    void (*callback)(struct sanic_http_request *, struct sanic_http_response *res);
#endif

    struct sanic_route *next;
    __attribute__((unused)) char **parts;
    __attribute__((unused)) size_t parts_count;
};

#ifdef USE_CLANG_BLOCKS
void sanic_http_on_get(const char *route, void (^callback)(struct sanic_http_request *req, struct sanic_http_response *res));
#else
void sanic_http_on_get(const char *route, void (*callback)(struct sanic_http_request *req, struct sanic_http_response *res));
#endif

int sanic_http_serve(uint16_t port);

#endif //SANIC_HTTP_H
