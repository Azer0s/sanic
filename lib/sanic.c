#include "include/sanic.h"

#include <gc.h>
#include <uuid4.h>

void sanic_init() {
  GC_init();
  uuid4_init();
}