#include "include/http_request.h"

char *sanic_get_path_params_value(struct sanic_http_request *request, char *key) {
  struct sanic_http_param *param = sanic_http_param_get(&request->path_param, key);
  if (param != NULL) {
    return param->value;
  }
  return "";
}

char *sanic_get_query_params_value(struct sanic_http_request *request, char *key) {
  struct sanic_http_param *param = sanic_http_param_get(&request->query_param, key);
  if (param != NULL) {
    return param->value;
  }
  return "";
}
