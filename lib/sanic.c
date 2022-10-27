#define DEFINE_LOG_LEVEL
#define DEFINE_PRINT_MU

#include "include/sanic.h"
#include "include/log.h"

#include <gc.h>
#include <uuid4.h>
#include <stdlib.h>

void sanic_init() {
  sanic_log_level = LEVEL_TRACE;

  if (pthread_mutex_init(&print_mu, NULL) != 0) {
    sanic_log_error("mutex init failed");
    exit(1);
  }

  GC_INIT();
  GC_allow_register_threads();
  GC_enable_incremental();

  uuid4_init();
}