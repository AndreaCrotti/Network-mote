#ifndef __GLUE_H
#define __GLUE_H

#include "util.h"

typedef struct fdglue_handler_t {
  void* p;
  void (*handle)(fdglue_handler_t* this);
} fdglue_handler_t;

typedef enum {
  FDGHT_READ,
  FDGHT_WRITE,
  FDGHT_ERROR
} fdglue_handle_type_t;

class (fdglue_t,
  struct fdglue_handlerlist_t {
    fdglue_handlerlist_t* next;
    int fd;
    fdglue_handle_type_t type;
    fdglue_handler_t hnd;
  }* handlers;
  int nfds;
  void (*setHandler)(fdglue_t* this, int fd, fdglue_handle_type_t const type, fdglue_handler_t const hnd);
  void (*listen)(fdglue_t* this, unsigned timeout);
);

fdglue_t* fdglue(fdglue_t* this);

#endif
