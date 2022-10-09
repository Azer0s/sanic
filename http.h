#ifndef SANIC_HTTP_H
#define SANIC_HTTP_H

#include <stdint.h>
#include <stddef.h>
#include "http_request.h"

struct sanic_route;

struct sanic_route {
    const char *path;
    void (^callback)(struct sanic_http_request *);
    struct sanic_route *next;
    char **parts;
    size_t parts_count;
};

void sanic_http_on_get(const char *route, void (^callback)(struct sanic_http_request *req));
int sanic_http_serve(uint16_t port);

#endif //SANIC_HTTP_H
