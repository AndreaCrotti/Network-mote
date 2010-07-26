#ifndef GLUE_H
#define GLUE_H

#include "util.h"

typedef struct fdglue_handler_t {
  void* p;
  void (*handle)(struct fdglue_handler_t* this);
} fdglue_handler_t;

typedef enum {
  FDGHT_READ = 0,
  FDGHT_WRITE = 1,
  FDGHT_ERROR = 2
} fdglue_handle_type_t;

#define FDGHT_SIZE 3

typedef enum {
  FDGHR_REPLACE = 0,
  FDGHR_APPEND = 1,
  FDGHR_REMOVE = 2
} fdglue_handler_replace_t;

#define FDGHR_SIZE 3
typedef struct fdglue_handlerlist_t {
  struct fdglue_handlerlist_t* next;
  int fd;
  fdglue_handle_type_t type;
  fdglue_handler_t hnd;
} fdglue_handlerlist_t;

class (fdglue_t,
  struct fdglue_handlerlist_t* handlers;
  int nfds;
  void (*setHandler)(fdglue_t* this, int fd, fdglue_handle_type_t const type, fdglue_handler_t const hnd, fdglue_handler_replace_t const action);
  void (*listen)(fdglue_t* this, unsigned timeout);
);

fdglue_t* fdglue(fdglue_t* this);

#endif
