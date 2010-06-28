// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __TIMEMANAGER_H__
#define __TIMEMANAGER_H__

#include "association.h"
#include <sys/time.h>
#include <stdbool.h>

bool timemanager_timeout_exceeded(const struct timeval);

#endif // __TIMEMANAGER_H__

#define timemanager_get_handshake_timeout() 1
#define timemanager_get_s1_timeout(a) (1)
