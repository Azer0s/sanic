#include "include/http_response.h"
#include "include/internal/http_param.h"

void sanic_http_header_insert(struct sanic_http_response *res, struct sanic_http_param *header) {
  sanic_http_param_insert(&res->headers, header);
}