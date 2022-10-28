#ifndef SANIC_REQUEST_UTIL_H
#define SANIC_REQUEST_UTIL_H

#include <uv.h>

struct sanic_http_request *sanic_read_request(uv_buf_t *buff, struct sanic_http_request *init_req);

#endif //SANIC_REQUEST_UTIL_H
