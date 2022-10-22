#ifndef SANIC_MIDDLEWARE_HANDLER_H
#define SANIC_MIDDLEWARE_HANDLER_H

#include "../middleware.h"

enum sanic_middleware_action sanic_handle_middlewares(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str);

#endif //SANIC_MIDDLEWARE_HANDLER_H
