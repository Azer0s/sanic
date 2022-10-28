#include "../include/internal/middleware_handler.h"
#include "../include/log.h"
#include "../include/internal/request_handler.h"

struct sanic_proceed_or_reply sanic_handle_middlewares(struct sanic_http_request *req, struct sanic_http_response *res, char *addr_str) {
  struct sanic_proceed_or_reply ret;

  sanic_fmt_log_trace_req(req, "processing middleware for %s", addr_str);

  struct sanic_middleware **current_middleware = &middlewares;
  while (*current_middleware != NULL) {
    enum sanic_middleware_action action = (*current_middleware)->callback(req, res);
    if (action == ACTION_STOP) {
      sanic_fmt_log_warn_req(req, "stopping request from %s due to middleware response", addr_str);
      ret.reply = true;
      ret.res = res;
      return ret;
    } else if (action == ACTION_REPLY) {
      if (res->status == -1) {
        res->status = 200;
      }

      sanic_fmt_log_info_req(req, "replying early to %s due to middleware response", addr_str);
      ret.reply = true;
      ret.res = res;
      return ret;
    }

    current_middleware = &(*current_middleware)->next;
  }

  ret.reply = false;
  return ret;
}