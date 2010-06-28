// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	signal.c
 * @brief	Signal handler used by ALPHA
 */

#include <signal.h>
#include "signal.h"

#include "defines.h"
#include "tools.h"
#include "host.h"
#include "system.h"
#include "xmalloc.h"

/** Signal handler for SIGHUP (invoked by add_host.pl and del_host.pl scripts)
 * @param	sig	the signal number, this is always SIGHUP
 */
void signal_hup(int sig) {
//[[
	statusmsg("Received SIGHUP! Ignoring. If you want alpha to reload its config, use alphacontrol.\n");
} //]]

/** Signal handler for SIGTERM and SIGINT (invoked by kill, Strg+C, etc.)
 * @param	sig	the signal number (this is always either SIGTERM or SIGINT)
 */
void signal_term(int sig) {
//[[
	puts("");

	if(sig == SIGTERM) {
		statusmsg("Received SIGTERM!\n");
	} else if(sig == SIGINT) {
		statusmsg("Received SIGINT\n");
	}

	exit(EXIT_FAILURE);

} //]]

/** atexit handler. This gets executed at normal process termination */
void byebye(int rc, void *arg) {
//[[

	// Free custom config file name, if one was given
	if(((config_t*)arg)->config_file != NULL) {
		xfree(((config_t*)arg)->config_file);
	}
	if(((config_t*)arg)->private_key != NULL) {
		DSA_free(((config_t*)arg)->private_key);
	}

	system_close_sockets((config_t*)arg);
	host_free_all();

} //]]

/** Initialize signal handler */
void signal_init(config_t *conf, void (*func)(void)) {
//[[

	// Restart system calls if interrupted by SIGHUP (for select())
	siginterrupt(SIGHUP, 0);

	// Signal handlers
	signal(SIGHUP,  signal_hup);
	signal(SIGTERM, signal_term);
	signal(SIGINT,  signal_term);

	// Exit handler (free stuff, etc.)
#ifndef HAVE_ON_EXIT
	atexit(func);
#else
	on_exit(byebye, conf);
#endif

} //]]
