#if SANIC_KQUEUE

#include <stdlib.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <uuid4.h>
#include <gc.h>
#include <sys/event.h>

#include "include/internal/sanic_ascii.h"
#include "include/log.h"
#include "include/internal/request_handler.h"
#include "include/internal/string_util.h"
#include "include/internal/server_internals.h"

volatile sig_atomic_t stop;
volatile int sock_fd;

char *sig2str(int signum) {
  return (char*) sys_signame[signum];
}

int sanic_http_serve(uint16_t port) {
  sanic_setup_interrupts(NULL, sig2str);
  int code = sanic_create_socket(port);

  if (code != 0) {
    return code;
  }

  sanic_log_debug("initializing kqueue");
  int kq = kqueue();
  struct kevent change_event[4], event[4];

  EV_SET(change_event, sock_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);

  if (kevent(kq, change_event, 1, NULL, 0, NULL) == -1) {
    //TODO: report error
    return 1;
  }

  struct connection_kqueue_data {
    char *req_id;
    struct sockaddr_in *conn_addr;
  };

  while (!stop) {
    int new_events = kevent(kq, NULL, 0, event, 4, NULL);
    if (new_events == -1) {
      //TODO: report error
      return 1;
    }

    for (int i = 0; i < new_events; ++i) {
      int event_fd = (int) event[i].ident;

      if (event_fd == sock_fd) {
        struct sockaddr_in *conn_addr = malloc(sizeof(struct sockaddr_in));
        size_t len = sizeof(*conn_addr);

        int conn_fd = accept(sock_fd, (struct sockaddr *) conn_addr, (socklen_t *) &len);

        char req_id[UUID4_LEN] = {0};
        bzero(req_id, 37);
        uuid4_generate(req_id);

        struct sanic_http_request tmp_request = {
          .req_id = req_id,
          .conn_fd = conn_fd
        };

        sanic_log_trace_req(&tmp_request, "accepted new connection");
        if (conn_fd < 0) {
          sanic_log_warn("server accept failed");
          continue;
        }

        struct connection_kqueue_data *conn_data = malloc(sizeof(struct connection_kqueue_data));;
        EV_SET(change_event, conn_fd, EVFILT_READ, EV_ADD, 0, 0, conn_data);

        if (kevent(kq, change_event, 1, NULL, 0, NULL) < 0) {
          //TODO: report error
          free(conn_data);
          free(conn_addr);
          continue;
        }

        conn_data->req_id = req_id;
        conn_data->conn_addr = conn_addr;
      } else if (event[i].filter & EVFILT_READ) {
        struct connection_kqueue_data *conn_data = event[i].udata;
        struct sanic_http_request tmp_request = {
          .req_id = conn_data->req_id,
          .conn_fd = event_fd
        };

        //TODO: split into read, exec and write
        sanic_handle_connection(conn_data->conn_addr, &tmp_request);
        //EV_SET(change_event, conn_fd, EVFILT_WRITE, EV_ADD, 0, 0, conn_data);

        free(conn_data->conn_addr);
        free(conn_data);
        GC_gcollect_and_unmap();

#if GC_DEBUG
        GC_dump();

        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
#endif
      }
    }
  }

  return 0;
}

#endif
