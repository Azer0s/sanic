#include "include/http_method.h"

const char *sanic_http_method_to_str(enum sanic_http_method method) {
  switch (method) {
    case METHOD_GET:
      return "GET";
    case METHOD_HEAD:
      return "HEAD";
    case METHOD_POST:
      return "POST";
    case METHOD_PUT:
      return "PUT";
    case METHOD_DELETE:
      return "DELETE";
    case METHOD_CONNECT:
      return "CONNECT";
    case METHOD_OPTIONS:
      return "OPTIONS";
    case METHOD_TRACE:
      return "TRACE";
    case METHOD_PATCH:
      return "PATCH";
    default:
      return "";
  }
}
