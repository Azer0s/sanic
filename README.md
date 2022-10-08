## sanic - Gotta go fast.

Sanic is a small HTTP framework built in C.

### Examples

```c
sanic_log_level = LEVEL_INFO;

sanic_http_on_get("/", ^void(struct sanic_http_request *req) {
    printf("Hello!\n");
});
```
