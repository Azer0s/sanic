#ifndef SANIC_SERVER_INTERNALS_H
#define SANIC_SERVER_INTERNALS_H

#include <uv.h>

#ifdef DEFINE_INTERNAL_VARS
volatile sig_atomic_t stop;
volatile int sock_fd;
#else
extern volatile sig_atomic_t stop;
extern volatile int sock_fd;
#endif

void sanic_setup_interrupts();
void sanic_stop_serve_internal();
char *sanic_sig2str(int signum);

#endif //SANIC_SERVER_INTERNALS_H
