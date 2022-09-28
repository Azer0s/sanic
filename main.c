#include "http.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  http_on_get("/", http_do({ printf("Hello!"); }));
  return 0;
}
