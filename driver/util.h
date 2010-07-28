#ifndef UTIL_H
#define UTIL_H

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

#define DUMP_VAR(VAR,TYPE) printf("[%s:%d] "#VAR" == %"#TYPE";\n",__FILE__,__LINE__,VAR)
#define DUMP_INT(VAR) DUMP_VAR(VAR,d)
#define DUMP_UINT(VAR) DUMP_VAR(VAR,u)
#define DUMP_LONG(VAR) DUMP_VAR(VAR,ld)
#define DUMP_ULONG(VAR) DUMP_VAR(VAR,lu)
#define DUMP_STRING(VAR) DUMP_VAR(VAR,s)
#define DUMP_FLOAT(VAR) DUMP_VAR(VAR,f)


//TODO: We need to find a better solution here...
#include "../shared/structs.h"

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
