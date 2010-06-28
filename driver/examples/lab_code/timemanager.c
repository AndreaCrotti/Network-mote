// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include "timemanager.h"

/* Check if timeout has been exceeded
 *
 * @param[in] timeout the time the timeout was started
 */
bool timemanager_timeout_exceeded(const struct timeval timeout) {
	struct timeval curtime;
	gettimeofday(&curtime, NULL);
	return (curtime.tv_sec > timeout.tv_sec || (curtime.tv_sec == timeout.tv_sec && curtime.tv_usec > timeout.tv_usec));
}
