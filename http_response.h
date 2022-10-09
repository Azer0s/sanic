#ifndef SANIC_HTTP_RESPONSE_H
#define SANIC_HTTP_RESPONSE_H

#include "http_header.h"

struct sanic_http_response {
    int status;
    char *response_body;
    __attribute__((unused)) struct sanic_http_header *headers;
};

#endif //SANIC_HTTP_RESPONSE_H
