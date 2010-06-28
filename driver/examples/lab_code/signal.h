// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __SIGNAL_H__
#define __SIGNAL_H__

// needed for config_t
#include "alpha.h"

void signal_hup(int sig);
void signal_term(int sig);
void signal_init(config_t *conf, void (*func)(void));
void byebye(int rc, void *arg);

#endif // __SIGNAL_H__
