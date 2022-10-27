#if SANIC_IO_URING

#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <uuid4.h>
#include <gc.h>
#include <fcntl.h>

#include "include/log.h"
#include "include/internal/server_internals.h"
#include "include/http_request.h"
#include "include/internal/request_handler.h"

struct sanic_epoll_connection_data {
  char *req_id;
  struct sockaddr_in *conn_addr;
  int conn_fd;
};

int epoll_fd;

void stop_epoll() {
  close(epoll_fd);
}

int sanic_http_serve(uint16_t port) {
  sanic_setup_interrupts(NULL, strsignal);
  int code = sanic_create_socket(port);

  if (code != 0) {
    return code;
  }

  sanic_log_debug("initializing epoll");

  epoll_fd = epoll_create(10);
  if (epoll_fd < 0) {
    //TODO: report error
    return 1;
  }

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &(struct epoll_event) {
    .events = EPOLLIN,
    .data.fd = sock_fd
  }) == -1) {
    //TODO: report error
    return 1;
  }

  struct epoll_event events[10];

  while (!stop) {
    int new_events = epoll_wait(epoll_fd, events, 10, -1);

    if (new_events == -1) {
      //TODO: report error
      return 1;
    }

    for (int i = 0; i < new_events; ++i) {
      if (events[i].data.fd == sock_fd) {
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

        struct sanic_epoll_connection_data *conn_data = malloc(sizeof(struct sanic_epoll_connection_data));
        conn_data->req_id = req_id;
        conn_data->conn_addr = conn_addr;
        conn_data->conn_fd = conn_fd;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &(struct epoll_event) {
          .events = EPOLLIN,
          .data.ptr = conn_data
        });
      }
      else if (events[i].events & EPOLLIN) {
        struct sanic_epoll_connection_data *conn_data = events[i].data.ptr;
        struct sanic_http_request tmp_request = {
          .req_id = conn_data->req_id,
          .conn_fd = conn_data->conn_fd
        };

        //TODO: split into read, exec and write
        sanic_handle_connection(conn_data->conn_addr, &tmp_request);
        //EV_SET(change_event, conn_fd, EVFILT_WRITE, EV_ADD, 0, 0, conn_data);

        free(conn_data->conn_addr);
        free(conn_data);
        GC_gcollect_and_unmap();
      }
    }
  }
}

#endif
