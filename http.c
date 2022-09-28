#include "http.h"
#include <stdlib.h>

struct route;

struct route {
  const char *path;
  void (*callback)(struct http_request *);
  struct route *next;
};

struct route *head = NULL;

void insert_route(struct route route) {
  if (head == NULL) {
    head = malloc(sizeof(struct route));
    head->path = route.path;
    head->callback = route.callback;
    return;
  }

  struct route *first_free = NULL;
  struct route *current = head;
  while (first_free == NULL) {
    if (current->next == NULL) {
      current->next = malloc(sizeof(struct route));
      first_free = current->next;
      break;
    }

    current = current->next;
  }

  first_free->path = route.path;
  first_free->callback = route.callback;
}

void http_on_get(const char *route, void (*callback)(struct http_request *)) {}
