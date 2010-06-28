// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

// Needed for struct sockaddr_in* in function prototypes below
#include <arpa/inet.h>

#include "alpha.h"

ap_err_code system_sockets_init(config_t *conf);
ap_err_code system_close_sockets(const config_t *conf);
ap_err_code system_set_mtu(const unsigned int mtu);

/** Set up the routing for alpha
 * @param	dest	network address of endhost (containing address and port)
 * @return	AP_ERR_SUCCESS on success, AP_ERR_SOCKET otherwise.
 *
 * (This function is implemented system specific, i.e. in system_tun.c or system_divert.c)
 */
ap_err_code system_addhost(const struct sockaddr_in *dest, const config_t* conf);

#endif /* SYSTEM_H_ */
