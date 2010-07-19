#ifndef __UTIL_H
#define __UTIL_H

#ifndef INCLUDE_SERIAL_IMPLEMENTATION
#define INCLUDE_SERIAL_IMPLEMENTATION 1
#endif

#ifndef DYNAMIC_MEMORY
#define DYNAMIC_MEMORY 1
#endif

#ifndef ALLOW_ASSERT
#define ALLOW_ASSERT 1
#endif

#if ALLOW_ASSERT
#include <assert.h>
#else
#define assert(expr) 
#endif

#include "structs.h"

#define class(NAME_T,body) \
  typedef struct NAME_T NAME_T;\
  struct NAME_T {\
    class_t _class;\
    body\
  }

#define forward(NAME_T) struct NAME_T

typedef void (*dtor_t)(void*);

#define CTOR(objPtr) _class_t_ctor((void**)&objPtr,sizeof(*objPtr))
#define DTOR(objPtr) _class_t_dtor((void**)&objPtr)
#define SETDTOR(classPtr) (classPtr)->dtor = (dtor_t)

#define virtual

typedef struct {
  char ctorAllocd;
  void (*dtor)(void* this);
} class_t;

class_t* _class_t_ctor(void** obj, unsigned typesz);
void _class_t_dtor(void** obj);

#endif
