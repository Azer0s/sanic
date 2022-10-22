#ifndef SANIC_CONNECTION_H
#define SANIC_CONNECTION_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include "../http_request.h"
#include "../http_response.h"

void sanic_finish_request(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str);
void sanic_handle_connection(int conn_fd, struct sockaddr_in conn_addr, struct sanic_http_request *init_req);

#endif //SANIC_CONNECTION_H
