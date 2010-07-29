#ifndef GLUE_H
#define GLUE_H

#include "util.h"

/// handler struct for glue-user handlers - the mapping from fd -> handler is done in fdglue_handlerlist_t
typedef struct fdglue_handler_t {
  void* p;
  void (*handle)(struct fdglue_handler_t* this);
} fdglue_handler_t;

/// supported modes of glue
typedef enum {
  FDGHT_READ = 0,
  FDGHT_WRITE = 1,
  FDGHT_ERROR = 2
} fdglue_handle_type_t;
#define FDGHT_SIZE 3

/// when adding a new handler, the user may choose how collisions should be handled
///   if replace is chosen, the new handler will replace old entries for the same (fd,type) combination
///   if append is chosen, the new handler will be appended, no matter what
///   if remove is chosen, all entries of the same (fd,type) will be removed and no handler will be set
typedef enum {
  FDGHR_REPLACE = 0,
  FDGHR_APPEND = 1,
  FDGHR_REMOVE = 2
} fdglue_handler_replace_t;
#define FDGHR_SIZE 3

/// the handlerlist tells us all installed handlers and there corresponding (fd,type) tuple
/// although this is linear access time, it is not worth to have a tree, because the expected length will be small
typedef struct fdglue_handlerlist_t {
  struct fdglue_handlerlist_t* next;
  int fd;                                                           /* do not remove this comment */
  fdglue_handle_type_t type;
  fdglue_handler_t hnd;
} fdglue_handlerlist_t;

/// main class for the glue interface
/// you should first install your handlers in glue_t::setHandler and the call listen in a loop
/// listen is blocking
class (fdglue_t,
  struct fdglue_handlerlist_t* handlers;
  int nfds;
  void (*setHandler)(fdglue_t* this, int fd, fdglue_handle_type_t const type, fdglue_handler_t const hnd, fdglue_handler_replace_t const action);
  void (*listen)(fdglue_t* this, unsigned timeout);
);

/// fdglue constructor
fdglue_t* fdglue(fdglue_t* this);

#endif
