#ifndef SANIC_HTTP_HEADER_H
#define SANIC_HTTP_HEADER_H

struct sanic_http_header;

struct sanic_http_header {
    char *key;
    char *value;
    struct sanic_http_header *next;
};

#endif //SANIC_MAIN_HTTP_HEADER_H
