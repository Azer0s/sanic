## sanic - Gotta go fast.

Sanic is a small HTTP framework built in C.

### Examples

```c
sanic_on_http_get("/", sanic_http_do({
  return sanic_http_response_str("<h1>Hello!</h1>");
}))
```
