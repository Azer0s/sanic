#ifndef SANIC_HTTP_RESPONSE_H
#define SANIC_HTTP_RESPONSE_H

struct sanic_http_response {
    int status;
    char *response_body;
    struct sanic_http_param *headers;
};

void sanic_http_header_insert(struct sanic_http_response *res, struct sanic_http_param *header);

#endif //SANIC_HTTP_RESPONSE_H
