// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	system.c
 * @brief	Platform-dependent functions
 */

#ifdef MACOSX
 #include <arpa/inet.h>
#endif

#include <errno.h>
#include <net/if.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#include "tools.h"
#include "system.h"

// Private prototypes
static ap_err_code system_open_sock_outgoing(int *sock, const unsigned int mtu);
static ap_err_code system_close_sock_outgoing(const config_t *conf);
static ap_err_code system_open_sock_incoming(int *sock, const config_t* conf);
static ap_err_code system_close_sock_incoming(const config_t *conf);
static ap_err_code system_open_sock_internal(int *sock);
static ap_err_code system_close_sock_internal(const config_t *conf);
static ap_err_code system_set_tun_mtu(const unsigned int mtu);

/********************** PORTABLE CODE ****************************/

/** Initialize the sockets
 * (UDP transport socket and the capturing device, that is divert sockets on Mac OS X and
 * a tun/tap device on linux)
 * @param	conf	Pointer to configuration structure, containing mtu and socket variables
 * @return	If one of the sockets fails to initialize, AP_ERR_SOCKET is returned, otherwise AP_ERR_SUCCESS
 */
ap_err_code system_sockets_init(config_t *conf) {
//[[

	// open the UDP socket for incoming packets
	if(system_open_sock_incoming(&conf->socket_incoming, conf) < 0) return AP_ERR_SOCKET;

	// set up the tun/tap device
	if(system_open_sock_outgoing(&conf->socket_outgoing, conf->mtu) < 0) return AP_ERR_SOCKET;

	// open internal socket
	if(system_open_sock_internal(&conf->socket_internal) < 0) return AP_ERR_SOCKET;

	return AP_ERR_SUCCESS;

} //]]

/** Close the sockets
 * @return	If one of the sockets fails to close, AP_ERR_SOCKET is returned, otherwise AP_ERR_SUCCESS
 */
ap_err_code system_close_sockets(const config_t *conf) {
//[[
	ap_err_code ret = AP_ERR_SUCCESS;

	if(system_close_sock_incoming(conf) != AP_ERR_SUCCESS) {
		ret = AP_ERR_SOCKET;
	}

	if(system_close_sock_outgoing(conf) != AP_ERR_SUCCESS) {
		ret = AP_ERR_SOCKET;
	}

	if(system_close_sock_internal(conf) != AP_ERR_SUCCESS) {
		ret = AP_ERR_SOCKET;
	}

	return ret;
} //]]

/** Set MTU of socket 
 * @return	If the socket fails to open, AP_ERR_SOCKET is returned, otherwise AP_ERR_SUCCESS
 */
ap_err_code system_set_mtu(const unsigned int mtu) {
	if(system_set_tun_mtu(mtu) < 0) return AP_ERR_SOCKET;
	return AP_ERR_SUCCESS;
}/*//]]*/

/** Open the socket ALPHA listens on
 * @param	sock	pointer to the socket
 * @param	conf	Configuration pointer
 * @return	AP_ERR_SUCCESS if opening succeeded, AP_ERR_SOCKET otherwise
 */
static ap_err_code system_open_sock_incoming(int *sock, const config_t* conf) {
 //[[

	statusmsg("Creating udp/%u socket\n", conf->port);
	if((*sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		print_error("socket(): %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(conf->port);

	if(bind(*sock, (struct sockaddr*)&sa, sizeof(struct sockaddr)) == -1) {
		print_error("bind(): %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;

} //]]

/** Close the socket ALPHA listens on
 * @param	conf	Configuration pointer
 * @return	AP_ERR_SUCCESS if closing succeeded, AP_ERR_SOCKET otherwise
 */
static ap_err_code system_close_sock_incoming(const config_t *conf) {
//[[
	statusmsg("Closing udp socket\n");

	if(close(conf->socket_incoming) == -1) {
		print_error("close() failed: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;
} //]]

/** Open the socket for communication with alphacontrol
 * @param	sock	pointer to the socket
 * @param	conf	Configuration pointer
 * @return	AP_ERR_SUCCESS if opening succeeded, AP_ERR_SOCKET otherwise
 */
static ap_err_code system_open_sock_internal(int *sock) {
//[[
	struct sockaddr_un local;

	if((*sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		print_error("Failed to create unix domain socket: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	if(strlen(SOCKETFILE)+1 > sizeof(local.sun_path)) {
		print_error("Failed to create unix domain socket: filename too long.\n");
		return AP_ERR_SOCKET;
	}

	local.sun_family = AF_UNIX;
	strcpy(local.sun_path, SOCKETFILE);

	// Delete the socket file, if it already exists
	unlink(local.sun_path);

	// Bind the socket
	if(bind(*sock, (struct sockaddr*)&local, sizeof(local)) == -1) {
		print_error("Failed to bind unix domain socket: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	return AP_ERR_SUCCESS;

} //]]

/** Close the socket for communication with alphacontrol
 * @param	conf	Configuration pointer
 * @return	AP_ERR_SUCCESS if opening succeeded, AP_ERR_SOCKET otherwise
 */
static ap_err_code system_close_sock_internal(const config_t *conf) {
//[[

	if(close(conf->socket_internal) == -1) {
		print_error("Failed to close unix domain socket: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	if(unlink(SOCKETFILE) == -1) {
		print_error("Failed to delete unix domain socket (%s): %s\n", SOCKETFILE, strerror(errno));
		return AP_ERR_SOCKET;
	}


	return AP_ERR_SUCCESS;
} //]]

/********************* UNPORTABLE CODE ***************************/

#ifdef MACOSX
 #include "system_divert.c"
#else
 #include "system_tun.c"
#endif
