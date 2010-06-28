// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifdef MACOSX
 #include <stdio.h>
#endif

#include <stdlib.h>

#define xmalloc(...)  _xmalloc(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define xrealloc(...) _xrealloc(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);
#define xfree(...)    _xfree(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__);

void fatal(const char *file, const unsigned int line, const char* function, const char *format, ...);
void* _xmalloc(const char *file, const int line, const char *function, size_t len);
void* _xrealloc(const char *file, const int line, const char *function, void *ptr, size_t size);
void  _xfree(const char *file, const int line, const char *function, void* ptr);
