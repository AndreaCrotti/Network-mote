#include "util.h"
#include <stdlib.h>

#ifndef NULL
#define NULL 0
#endif

class_t* _class_t_ctor(void** obj, unsigned typesz) {
  class_t* result = NULL;
  assert(obj);
  {
    char ctorAllocd = !*obj;
#if DYNAMIC_MEMORY
    if (ctorAllocd)
      *obj = malloc(typesz);
#else
    assert(!ctorAllocd);
#endif
    result = (class_t*)*obj;
    // in a class the class_t struct is always at the first address
    result->ctorAllocd = ctorAllocd;
    result->dtor = NULL;
  }
  return result;
}
void _class_t_dtor(void** obj) {
  assert(obj);
  {
    class_t* c = (class_t*)*obj;
    if (c->dtor) {
      c->dtor(*obj);
    }
#if DYNAMIC_MEMORY
    if (c->ctorAllocd) {
      free(*obj);
      *obj = NULL;
    }
#else
    assert(!(c->ctorAllocd));
#endif
  }
}
