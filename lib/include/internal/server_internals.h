#ifndef SANIC_SERVER_INTERNALS_H
#define SANIC_SERVER_INTERNALS_H

#ifdef DEFINE_INTERNAL_VARS
volatile sig_atomic_t stop;
volatile int sock_fd;
#else
extern volatile sig_atomic_t stop;
extern volatile int sock_fd;
#endif

void sanic_setup_interrupts(void (*stop_cb)(void), char *(*sig2str_cb)(int));
int sanic_create_socket(uint16_t port);

#endif //SANIC_SERVER_INTERNALS_H
