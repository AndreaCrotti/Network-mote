// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	system_divert.c
 * @brief	Platform-dependent functions for OS X
 */

#include "host.h"

///////////////////////////////////////////
// THIS FILE GETS INCLUDED FROM SYSTEM.C //
///////////////////////////////////////////

/*********************** PUBLIC FUNCTIONS *****************************/

ap_err_code system_addhost(const struct sockaddr_in *dest, const config_t* conf) {
//[[

	// TODO: Use ipfw API (google "struct ip_fw") instead of system() calls

	// Install firewall rules
	statusmsg("Installing ipfw divert rules for %s\n", inet_ntoa(dest->sin_addr));

	//
	// WARNING: THIS IS ALL VERY EXPERIMENTAL.
	//

	char buf[SYS_BUFSIZE];

	snprintf(buf, SYS_BUFSIZE, "%s %s add %u divert %u ip from me not %u to %s", SYS_PREFIX,
		SYS_IPFW, SYS_IPFW_RULE, SYS_DIVERT_PORT, conf->port, inet_ntoa(dest->sin_addr));

	if(system(buf) == -1) {
		print_error("system(): Failed to add routing table entry for %s. Aborting.\n", inet_ntoa(dest->sin_addr));
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;

} //]]

/********************** PRIVATE FUNCTIONS *****************************/

/** Open a divert socket
 * @param	sock	Pointer to the variable where the sockets file descriptor should get saved to.
 * @param	mtu	This is ignored on Mac OS X.
 * @return	AP_ERR_SOCKET if something fails, AP_ERR_SUCCESS otherwise.
 */
static ap_err_code system_open_sock_outgoing(int *sock, const unsigned int mtu) {
//[[

	// port to which the fw will send the diverted packets
	struct sockaddr_in bind_port;

	// Open a divert socket
	if((*sock = socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT)) == -1) {
		print_error("socket(): %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	statusmsg("Binding divert socket to port %d.\n", SYS_DIVERT_PORT);

	// Set port
	bind_port.sin_family = AF_INET;
	bind_port.sin_port = htons(SYS_DIVERT_PORT);
	bind_port.sin_addr.s_addr = 0;

	if((bind(*sock, (struct sockaddr *)&bind_port, sizeof(bind_port))) != 0) {
		print_error("Failed to bind divert socket to port %u\n", SYS_DIVERT_PORT);
		close(*sock);
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;

} //]]

/** Close the divert socket (and clean up the routes for each client)
 * @return	If close() fails or cleaning up the routing for one of the clients fails,
 *		AP_ERR_SOCKET is returned, otherwise AP_ERR_SUCCESS.
 */
static ap_err_code system_close_sock_outgoing(const config_t *conf) {
//[[

	ap_err_code ret = AP_ERR_SUCCESS;

	//
	// WARNING: This is still kind of messy!
	//

	statusmsg("Removing ipfw divert rules for all %u clients.\n", hosts_len());
	char buf[SYS_BUFSIZE];
	unsigned int i;
	for(i=0; i<hosts_len(); i++) {

		snprintf(buf, SYS_BUFSIZE, "%s %s delete %u", SYS_PREFIX, SYS_IPFW, SYS_IPFW_RULE);
		if(system(buf) == -1) {
			print_error("Cleaning up ipfw rules for client %d failed.\n", i);
			ret = AP_ERR_SOCKET;
		}

	}

	if(close(conf->socket_outgoing) == -1) {
		print_error("close(): %s\n", strerror(errno));
		ret = AP_ERR_SOCKET;
	}

	return ret;

} //]]
