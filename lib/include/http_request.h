#ifndef SANIC_HTTP_REQUEST_H
#define SANIC_HTTP_REQUEST_H

#include "http_header.h"
#include "http_method.h"

struct sanic_http_request {
    enum sanic_http_method method;
    char *path;
    char *version;
    char *req_id;
    struct sanic_http_header *headers;
    int conn_fd;
};

char *sanic_get_params_value(struct sanic_http_request *request, const char *key);

#endif //SANIC_HTTP_REQUEST_H
