#ifndef SANIC_CONNECTION_H
#define SANIC_CONNECTION_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>

#include "../http_request.h"
#include "../http_response.h"

struct sanic_proceed_or_reply {
  bool reply;
  struct sanic_http_request *req;
  struct sanic_http_response *res;
};

void sanic_finish_request(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str);
struct sanic_proceed_or_reply sanic_handle_connection_read(const uv_buf_t *buff, struct sanic_http_request *init_req, char *addr_str);
void sanic_handle_connection_make_response(struct sanic_http_request *req, struct sanic_http_response *res);

#endif //SANIC_CONNECTION_H
