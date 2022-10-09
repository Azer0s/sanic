## sanic - Gotta go fast.

Sanic is a simple, small, express-ish HTTP framework built in C.

### Example - clang blocks

Originally, I built sanic with clang blocks in mind. So they're supported out of the box.

```c
sanic_log_level = LEVEL_INFO;

sanic_http_on_get("/", ^void(struct sanic_http_request *req) {
    printf("Hello!\n");
});

sanic_http_on_get("/people/{:name}", ^void(struct sanic_http_request *req) {
    printf("Hello %s!\n", sanic_get_params_value(request, "name"));
});

return sanic_http_serve(8080);
```

### Example - function pointers

A less elegant solution is to just pass handler functions as callbacks. With gcc, this can be done in a lambda-ish way,
although I haven't found out how to make cmake understand that.

```c
sanic_http_on_get("/", handle_index);
sanic_http_on_get("/people/{:name}", handle_get_person);

return sanic_http_serve(8080);
```

```c
// This should work in GNU C

sanic_http_on_get("/", ({
    void _(struct sanic_http_request *req) {
        printf("Hello!\n");
    }
    _;
}));

sanic_http_on_get("/people/{:name}", ({
    void _(struct sanic_http_request *req) {
        printf("Hello %s!\n", sanic_get_params_value(request, "name"));
    }
    _;
}));

return sanic_http_serve(8080);
```

### Middlewares

sanic supports middlewares out of the box. These can either act as filters or as blockers for any request.

```c
sanic_use_middleware((struct sanic_middleware) {
          .callback = ^enum sanic_middleware_action(struct sanic_http_request *req, struct sanic_http_response *res) {
              if (strcmp(req->path, "/foobar") == 0) {
                res->status = 300;
                return ACTION_STOP;
              }
              return ACTION_PASS;
          }
  });
```