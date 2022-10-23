#ifndef SANIC_HTTP_REQUEST_H
#define SANIC_HTTP_REQUEST_H

#include <stddef.h>
#include "http_method.h"
#include "internal/http_param.h"

struct sanic_http_request {
    char *req_id;
    int conn_fd;

    char *path;
    char **path_parts;
    size_t path_len;

    enum sanic_http_method method;
    char *version;

    struct sanic_http_param *path_param;
    struct sanic_http_param *query_param;
    struct sanic_http_param *headers;

    char *body;
};

char *sanic_path_params_get(struct sanic_http_request *request, char *key);
char *sanic_query_params_get(struct sanic_http_request *request, char *key);

#endif //SANIC_HTTP_REQUEST_H
