#ifndef SANIC_HTTP_REQUEST_H
#define SANIC_HTTP_REQUEST_H

#include <stddef.h>
#include "http_header.h"
#include "http_method.h"
#include "internal/http_param.h"

struct sanic_http_request {
    enum sanic_http_method method;
    char *path;
    char **path_parts;
    size_t path_len;
    struct sanic_http_param *path_param;
    struct sanic_http_param *query_param;
    char *version;
    struct sanic_http_header *headers;
    char *req_id;
    int conn_fd;
};

char *sanic_get_path_params_value(struct sanic_http_request *request, char *key);
char *sanic_get_query_params_value(struct sanic_http_request *request, char *key);

#endif //SANIC_HTTP_REQUEST_H
