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

#if LOG_LEVEL&128
#define DUMP_VAR(VAR,TYPE) printf("[%s:%d] "#VAR" == %"#TYPE";\n",__FILE__,__LINE__,VAR)
#else
#define DUMP_VAR(VAR,TYPE)
#endif
#define DUMP_CHAR(VAR) DUMP_VAR((int)VAR,d)
#define DUMP_UCHAR(VAR) DUMP_VAR((unsigned)VAR,u)
#define DUMP_INT(VAR) DUMP_VAR(VAR,d)
#define DUMP_UINT(VAR) DUMP_VAR(VAR,u)
#define DUMP_LONG(VAR) DUMP_VAR(VAR,ld)
#define DUMP_ULONG(VAR) DUMP_VAR(VAR,lu)
#define DUMP_STRING(VAR) DUMP_VAR(VAR,s)
#define DUMP_FLOAT(VAR) DUMP_VAR(VAR,f)

#define CONTROL "27[0;"
#define ERROR_COLOR "31m"
#define INFO_COLOR "34m"
#define WARNING_COLOR "32m"

// log levels
#include <stdio.h>
#define LOG__PRINT(TYPE,MSG,...) {fprintf(stderr,"%-7s " MSG "\n","<" TYPE ">",##__VA_ARGS__); fflush(stderr); }

#if LOG_LEVEL&1
#define LOG_ERROR(MSG,...)   LOG__PRINT(CONTROL ERROR_COLOR "ERROR",MSG,##__VA_ARGS__)
#else
#define LOG_ERROR(MSG,...)
#warning "error messages not compiled"
#endif

#if LOG_LEVEL&2
#define LOG_WARNING(MSG,...) LOG__PRINT("WARN",MSG,##__VA_ARGS__)
#else
#define LOG_WARNING(MSG,...)
#endif
#define LOG_WARN LOG_WARNING

#if LOG_LEVEL&4
#define LOG_NOTE(MSG,...)    LOG__PRINT("NOTE",MSG,##__VA_ARGS__)
#else
#define LOG_NOTE(MSG,...)
#endif

#if LOG_LEVEL&8
#define LOG_INFO(MSG,...)   LOG__PRINT(CONTROL INFO_COLOR "INFO",MSG,##__VA_ARGS__)
#else
#define LOG_INFO(MSG,...)
#endif

#if LOG_LEVEL&16
#define LOG_DEBUG(MSG,...)  LOG__PRINT("DEBUG",MSG,##__VA_ARGS__)
#else
#define LOG_DEBUG(MSG,...)
#endif


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
