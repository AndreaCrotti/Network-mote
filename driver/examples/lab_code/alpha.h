// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file alpha.h Abstraction from the different alpha mode
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * @date April 2009
 *
 * This file includes some typedefs which are used by the different alpha modes.
 *
 * @see alpha_n.h, alpha_m.h, alpha_c.h
 */

#ifndef __ALPHA_H_
#define __ALPHA_H_

#include "defines.h"
#include <stdbool.h>
#include <openssl/dsa.h>

/** Describes the currently running alpha mode */
typedef enum alpha_mode { ALPHA_N, ALPHA_M, ALPHA_C, ALPHA_Z } alpha_mode_t;

/** Structure holding the global configuration parameters */
typedef struct {
	/* System parameters */
	int socket_incoming;
	int socket_outgoing;
	int socket_internal;
	unsigned int mtu;
	unsigned short port;
	bool autoroute;
	bool daemon;
	char* config_file;
	int connect_secret;
	DSA* private_key;

	/* Protocol parameters */
	int num_alpha_n;
	int num_alpha_c;
	int num_alpha_m;
	int num_alpha_z;
	int alpha_m_sec_mode;

	/* Development parameters */
	int hchain_length;
} config_t;

#endif // __ALPHA_H_
