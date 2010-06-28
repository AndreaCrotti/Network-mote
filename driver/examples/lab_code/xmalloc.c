// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	xmalloc.c
 * @brief	malloc-wrappers for catching failures when allocating memory
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xmalloc.h"
#include "tools.h"

/** wrapper to handle fatal errors and end the programm immediately
 * @param	file		file the error occured in
 * @param	line		line the error occured in
 * @param	function	function the error occured in
 * @param	format		format string (printf syntax)
 */
void fatal(const char *file, const unsigned int line, const char* function, const char *format, ...) {
//[[

	va_list ap;
	va_start(ap, format);

	if(isatty(STDERR_FILENO)) {
		fprintf(stderr, "\033[31;;31mFATAL\033[0;;0m");
	} else {
		fprintf(stderr, "FATAL");
	}

	if(file) {
		fprintf(stderr, " [%s:%d:%s]: ", file, line, function);
	} else {
		fprintf(stderr, ": ");
	}

	vfprintf(stderr, format, ap);
	va_end(ap);
	exit(EXIT_FAILURE);

} //]]

/** malloc wrapper to automatically catch failed allocs
 * @param	file		file the malloc occured in
 * @param	line		line the malloc occured in
 * @param	function	function the malloc occured in
 * @param	size		bytes to malloc
 */
void *_xmalloc(const char *file, const int line, const char *function, size_t size) {
//[[ 
	// TODO: Do something smart when we run out of memory, like freeing stale hosts
	void *ptr;

	if (size == 0) {
		fatal(file, line, function, "xmalloc: zero size\n");
	}

	ptr = malloc(size);

	if (ptr == NULL) {
		fatal(file, line, function, "xmalloc: out of memory (allocating %lu bytes)\n", size);
	}

	return ptr;

} //]]

/** realloc wrapper to automatically catch failed reallocs
 * @param	file		file the realloc occured in
 * @param	line		line the realloc occured in
 * @param	function	function the realloc occured in
 * @param	ptr		pointer to currect buffer
 * @param	size		new size in bytes
 */
void *_xrealloc(const char *file, const int line, const char *function, void *ptr, size_t size) {
//[[
	void *new_ptr;

	if (size == 0) {
		fatal(file, line, function, "xrealloc: zero size\n");
	}

	if (ptr == NULL) {
		new_ptr = malloc(size);
	} else {
		new_ptr = realloc(ptr, size);
	}

	if (new_ptr == NULL) {
		fatal(file, line, function, "xrealloc: out of memory (size %lu bytes)\n", size);
	}

	return new_ptr;

} //]]

/** free wrapper to automatically catch failed frees
 * @param	file		file the free occured in
 * @param	line		line the free occured in
 * @param	function	function the free occured in
 * @param	ptr		pointer to buffer
 */
void _xfree(const char *file, const int line, const char *function, void* ptr) {
//[[
	if (ptr == NULL) {
		_print_error(file, line, "xfree: NULL pointer given as argument\n");
	}

	free(ptr);

} //]]
