// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	control.c
 * @brief	Helper functions for communicating with alphacontrol
 */

#include <sys/un.h>
#include <errno.h>

#include "tools.h"
#include "xmalloc.h"
#include "host.h"
#include "alphacontrol.h"
#include "control.h"

/** Send a list of our known clients to alphacontrol
 * @param	conf	Pointer to the configuration options
 * @param	to	Socket address of the unix domain socket
 * @param	tolen	Length of the address (gets set by recvfrom())
 * @return	AP_ERR_SOCKET if sendto() fails, AP_ERR_NOMEM if xmalloc() fails, AP_ERR_SUCCESS otherwise
 */
ap_err_code control_send_clientlist(const config_t *conf, struct sockaddr_un *to, socklen_t tolen) {
//[[

	int hosts = hosts_len();
	int len = sizeof(internal_packet_response_client) + hosts * sizeof(struct response_client_data);
	internal_packet_response_client *packet = xmalloc(len);

	packet->type = INTERNAL_CLIENTLIST;
	packet->len = hosts;

	struct response_client_data *c = (struct response_client_data*)( (char*)packet + sizeof(internal_packet_response_client) );
	int i;
	for(i=0; i<hosts; i++) {
		struct in_addr ip = host_get_addr(i);
		c->id = i;
		c->ip = ip.s_addr;
		c++;
	}

	if(sendto(conf->socket_internal, (char*)packet, len, 0, (struct sockaddr*)to, tolen) != len) {
		xfree(packet);
		print_error("Failed to respond to control message: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	xfree(packet);

	return AP_ERR_SUCCESS;

} //]]

/** Send our VERSION string to alphacontrol using the internal unix domain socket
 * @param	conf	Pointer to the configuration options
 * @param	to	Socket address of the unix domain socket
 * @param	tolen	Length of the address (gets set by recvfrom())
 * @return	AP_ERR_SOCKET if sendto() fails, AP_ERR_NOMEM if xmalloc() fails, AP_ERR_SUCCESS otherwise
 */
ap_err_code control_send_version(const config_t *conf, struct sockaddr_un *to, socklen_t tolen) {
//[[

	int len = sizeof(internal_packet) + strlen(VERSION) + 1;

	internal_packet *packet;
	packet = xmalloc(len);

	packet->type = INTERNAL_VERSION;
	strcpy((char*)packet + sizeof(internal_packet), VERSION);

	if(sendto(conf->socket_internal, packet, len, 0, (struct sockaddr*)to, tolen) != len) {
		xfree(packet);
		print_error("Failed to respond to control message: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	xfree(packet);

	return AP_ERR_SUCCESS;

} //]]
