#ifndef SANIC_REQUEST_H
#define SANIC_REQUEST_H

struct sanic_http_header;

struct sanic_http_header {
    char *key;
    char *value;
    struct sanic_http_header *next;
};

enum sanic_http_method {
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    CONNECT,
    OPTIONS,
    TRACE,
    PATCH
};

struct sanic_http_request {
    enum sanic_http_method method;
    char *path;
    char *version;
    struct sanic_http_header *headers;
};

struct sanic_http_request *sanic_read_request(int fd);

#endif //SANIC_REQUEST_H
