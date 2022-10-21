#ifndef SANIC_MIDDLEWARE_H
#define SANIC_MIDDLEWARE_H

#include "http_response.h"
#include "http_request.h"

enum sanic_middleware_action {
    ACTION_STOP,
    ACTION_PASS
};

struct sanic_middleware;

struct sanic_middleware {
#ifdef USE_CLANG_BLOCKS
    enum sanic_middleware_action (^callback)(struct sanic_http_request *, struct sanic_http_response *);
#else
    enum sanic_middleware_action (*callback)(struct sanic_http_request *, struct sanic_http_response *);
#endif

    struct sanic_middleware *next;
};

#ifdef DEFINE_MIDDLEWARES
struct sanic_middleware *middlewares = NULL;
#else
extern struct sanic_middleware *middlewares;
#endif

void sanic_use_middleware(
  #ifdef USE_CLANG_BLOCKS
  enum sanic_middleware_action (^callback)(struct sanic_http_request *, struct sanic_http_response *)
  #else
  enum sanic_middleware_action (*callback)(struct sanic_http_request *, struct sanic_http_response *)
  #endif
);

#endif //SANIC_MIDDLEWARE_H
