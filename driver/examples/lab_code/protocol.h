// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __PROTOCOL_H_
#define __PROTOCOL_H_

#include "hashtree.h"

socklen_t ap_protocol_send_udp(const config_t *conf, const int client_id, const void *buf, const size_t len);
socklen_t ap_protocol_send_syn(const config_t *conf, const int client_id);
socklen_t ap_protocol_send_ack(const config_t *conf, const int client_id);
socklen_t ap_protocol_send_ackack(const config_t *conf, const int client_id);
socklen_t ap_protocol_send_s2(const config_t *conf, const int client_id, const void *buf, const size_t len);
socklen_t ap_protocol_send_a1(const config_t *conf, const int client_id);
void ap_protocol_process_udp_packet(const config_t *conf, const int client_id, alpha_packet_t *packet, const int bytes);
socklen_t ap_protocol_send_connect(const config_t *conf, const int client_id);
void ap_protocol_handle_connect(const config_t *conf, const struct sockaddr_in *fromaddr, const socklen_t *fromlen);
int ap_protocol_verify_syn(const config_t *conf, const alpha_packet_syn_t *packet, const struct sockaddr_in *fromaddr);
int timeslot(int offset);

// quick struct for packaging the information to be signed
typedef struct {
	uint32_t	challenge;
	unsigned char	sign_anchor[HASHSIZE];
	unsigned char	ack_anchor[HASHSIZE];
	unsigned char	return_sign_anchor[HASHSIZE];
	unsigned char	return_ack_anchor[HASHSIZE];
} __attribute__((__packed__)) challenge_response_t;

#endif // __PROTOCOL_H_
