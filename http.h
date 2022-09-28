
struct http_request {};

#define http_do(action)                                                        \
  ({                                                                           \
    void __fn__(struct http_request *req) action;                              \
    __fn__;                                                                    \
  })

void http_on_get(const char *route, void (*callback)(struct http_request *req));
