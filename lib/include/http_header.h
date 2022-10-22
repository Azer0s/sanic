#ifndef SANIC_HTTP_HEADER_H
#define SANIC_HTTP_HEADER_H

struct sanic_http_header;

struct sanic_http_header {
    char *key;
    char *value;
    struct sanic_http_header *next;
};

void sanic_http_header_insert(struct sanic_http_header **headers, struct sanic_http_header *header);

#endif //SANIC_HTTP_HEADER_H
