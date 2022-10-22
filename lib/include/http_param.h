#ifndef SANIC_HTTP_PARAM_H
#define SANIC_HTTP_PARAM_H

struct sanic_http_param;

struct sanic_http_param {
    char *key;
    char *value;
    struct sanic_http_param *next;
};

void sanic_http_param_insert(struct sanic_http_param **params, struct sanic_http_param *param);


#endif //SANIC_HTTP_PARAM_H
