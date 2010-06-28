// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	tools.c
 * @brief	tools and helper functions used by the alpha daemon
 * @author	Florian Weingarten, Johannes Gilger
 */

#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include "tools.h"
#include "alpha.h"

// consult strftime(2)
// #define LOG_TIME_FORMAT "%b %d %H:%M:%S alpha: "
#define LOG_TIME_FORMAT "%T "

// Prefix the log messages (i.e. with a timestamp)
static void prefix(FILE *f) {
//[[
	char buf[10];
	struct tm *t;
	time_t tt = time(NULL);

	if((t = localtime(&tt)) == NULL) {
		return;
	}

	if(strftime(buf, sizeof(buf), LOG_TIME_FORMAT, t) == 0) {
		return;
	}

	fprintf(f, "%s", buf);

} //]]

/** Print a (formatted) status message
 * @param	format	the format string and its arguments (printf syntax)
 */
void statusmsg(const char *format, ...) {
//[[
	prefix(stdout);
	va_list ap;
	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
} //]] 

/** Print a (formatted) error message with red highlight
 * @param	file	source-file the error occured in
 * @param	line	line in source-file the error occured in
 * @param	format	the format string and its arguments (printf syntax)
 */
void _print_error(const char *file, const unsigned int line, const char *format, ...) {
//[[ 

	prefix(stderr);

	va_list ap;
	va_start(ap, format);

	// Only use colors when output goes to a terminal
	if(isatty(STDERR_FILENO)) {
		fprintf(stderr, "\033[31;;31mERROR\033[0;;0m");
	} else {
		fprintf(stderr, "ERROR");
	}

	if(file) {
		fprintf(stderr, " [%s:%d]: ", file, line);
	} else {
		fprintf(stderr, ": ");
	}

	vfprintf(stderr, format, ap);
	va_end(ap);
} //]]

/** Print command line usage of alpha binary
 * @param	me	file name of binary (usually argv[0])
 */
void print_usage(const char *me) {
//[[
	printf(" ALPHA (version %s)\n", VERSION);
	printf(	" Usage: %s [-a] [-d] [-c config] [-l length] [-n n:c:m] \n"
		"  -a: DO NOT auto-configure routing for known hosts (overrides config file settings)\n"
		"  -d: Run alpha as daemon in the background\n"
		"  -c: use config-file (overrides files found in . or %s)\n\n"
		" Development:\n"
		"  -l: Use hash chains of this length (default 10000)\n"
		"  -n: Start N, C, M many outgoing associations of the corresponding mode\n"
		"\n"
		" Options on the command line override directives in the config-file\n",
	me, CONFIGPATH);
} //]]
