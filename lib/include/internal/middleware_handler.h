#ifndef SANIC_MIDDLEWARE_HANDLER_H
#define SANIC_MIDDLEWARE_HANDLER_H

#include "../middleware.h"

struct sanic_proceed_or_reply sanic_process_middlewares(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str);

#endif //SANIC_MIDDLEWARE_HANDLER_H
