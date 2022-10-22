#ifndef SANIC_HTTP_METHOD_H
#define SANIC_HTTP_METHOD_H

enum sanic_http_method {
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_TRACE,
    METHOD_PATCH
};

const char *sanic_http_method_to_str(enum sanic_http_method method);

#endif //SANIC_HTTP_METHOD_H
