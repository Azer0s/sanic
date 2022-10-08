#include "http.h"
#include "request.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

struct sanic_route *routes = NULL;

void insert_route(struct sanic_route route) {
  struct sanic_route **current = &routes;
  while (*current != NULL) {
    current = &(*current)->next;
  }

  *current = malloc(sizeof(struct sanic_route));
  (*current)->path = route.path;
  (*current)->callback = route.callback;
}

void sanic_http_on_get(const char *path, void (^callback)(struct sanic_http_request *)) {
  struct sanic_route route;
  route.path = path;
  route.callback = callback;
  insert_route(route);
}

volatile sig_atomic_t stop;
volatile int sock_fd;

void sig_handler(int signum) {
  printf("shutting down server\n");
  close(sock_fd);
  stop = 1;
  exit(0);
}

int sanic_http_serve(uint16_t port) {
  signal(SIGTERM, sig_handler);
  signal(SIGKILL, sig_handler);

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    fprintf(stderr, "socket creation failed\n");
    return 1;
  }
  printf("socket successfully created\n");

  struct sockaddr_in sock_addr;
  sock_addr.sin_family = AF_INET;
  sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  sock_addr.sin_port = htons(port);

  if (bind(sock_fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) != 0) {
    fprintf(stderr, "socket bind failed\n");
    return 1;
  }
  printf("socket bind successful\n");

  if (listen(sock_fd, 128) != 0) {
    fprintf(stderr, "listen failed\n");
    return 1;
  }
  printf("server listening\n");

  while (!stop) {
    struct sockaddr_in conn_addr;
    size_t len = sizeof(conn_addr);
    int conn_fd = accept(sock_fd, (struct sockaddr *) &conn_addr, (socklen_t *) &len);
    if (conn_fd < 0) {
      fprintf(stderr, "server accept failed\n");
      break;
    }

    char addr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(conn_addr.sin_addr), addr_str, INET_ADDRSTRLEN);
    printf("serving %s\n", addr_str);

    struct sanic_http_request *request = sanic_read_request(conn_fd);
    free(request);

    FILE *conn_file = fdopen(conn_fd, "w+");
    fprintf(conn_file, "HTTP/1.1 200 OK\nConnection: Closed\n\n<h1>Hello</h1>");
    fflush(conn_file);

    close(conn_fd);
  }

  return 0;
}
