#ifndef SANIC_HTTP_REQUEST_H
#define SANIC_HTTP_REQUEST_H

#include "http_header.h"
#include "http_method.h"

struct sanic_http_request {
    enum sanic_http_method method;
    char *path;
    char *version;
    struct sanic_http_header *headers;
};

struct sanic_http_request *sanic_read_request(int fd);

#endif //SANIC_HTTP_REQUEST_H