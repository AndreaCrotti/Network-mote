#include "glue.h"
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

void _fdglue_t_setHandler(fdglue_t* this, int fd, fdglue_handle_type_t const type, fdglue_handler_t const hnd) {
  fdglue_handlerlist_t dummy;
  dummy.fd = -1;
  dummy.next = this->handlers;
  struct fdglue_handlerlist_t* it;
  for (it = &dummy; it; it = it->next) {
    if (it->fd == fd) {
      it->type = type;
      it->hnd;
      return;
    }
  }
  assert(DYNAMIC_MEMORY);
  it->next = malloc(sizeof(fglue_handlerlist_t));
  it->next->fd = fd;
  it->next->type = type;
  it->next->hnd = hnd;
  it->next->next = NULL;
  if (fd > nfds)
    nfds = fd;
  if (it == &dummy) {
    this->handlers = dummy.next;
  }
}
/*
class (fdglue_t,
  struct fdglue_handlerlist_t {
    fdglue_handlerlist_t* next;
    int fd;
    fdglue_handle_type_t type;
    fdglue_handler_t hnd;
  }* handlers;
  void (*setHandler)(fdglue_t* this, int fd, fdglue_handle_type_t const type, fdglue_handler_t const hnd);
  void (*listen)(fdglue_t* this, unsigned timeout);
);*/

void _fdglue_t_listen(fdglue_t* this, unsigned timeout) {
  fd_set rd, wr, er;
  FD_ZERO(&rd);
  FD_ZERO(&wr);
  FD_ZERO(&er);
  struct fdglue_handlerlist_t* it;
  for ()
}

fdglue_t* fdglue(fdglue_t* this) {
  CTOR(this);
  this->handlers = NULL;
  this->nfds = -1;
  this->setHandler = _fdglue_t_setHandler;
  this->listen = _fdglue_t_listen;
  return this;
}
