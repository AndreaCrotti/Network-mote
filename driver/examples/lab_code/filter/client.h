// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __FILTERHOST_H_
#define __FILTERHOST_H_

// Include definitions of alpha modes (enums suck!) <== NO THEY DO NOT!
#include "../alpha.h"
#include "../packet.h"
#include "../ring_buffer.h"

#include "association_relay.h"

#define NUM_ASSS 256

// This is one "fragment" of payload in alpha_packet_new_ass_t payload
struct association_payload {
	uint8_t new_ass_id;
	uint8_t mode;
	uint8_t sign_anchor[HASHSIZE];
	uint8_t ack_anchor[HASHSIZE];
};

// This structure represents an client in a client pair of an alpha connection
// between two endpoints
struct alpha_client_relay {

	// We identify relay_clients by address and port, even though port
	// will be useless at the moment, because at the current ALPHA
	// implementation, everbody HAS TO use the same port. But that may
	// change in the future
	struct in_addr addr;
	unsigned short port;

	// The associations of this client in the selected client pair
	struct association_relay** associations;

};

// This struct holds all known connections, identified by
// the addresses of the two communicating partners.
// As a convention, store the client with the smaller
// address first.
// Note that one and the same client (i.e. the same struct in_addr)
// can appear in different connections (different client pairs),
// and in each of those pairs, there has to be another
// struct alpha_client entry, because the anchors will not be
// the same!
struct s_client_pair {

	// handshake already done? 0 if not, something else otherwise
	unsigned char handshake_done;

	// the two endpoints (the one with the smaller ip first)
	struct alpha_client_relay c[2];

};

int client_add_pair(struct alpha_client_relay first, struct alpha_client_relay second);
struct association_relay* client_find_association(int client_pair_id, struct in_addr sender, unsigned int ass_id);
void client_add_association(unsigned int client_pair_id, struct in_addr sender, struct association_relay* relay);
int client_find_pair(struct in_addr first, unsigned short first_port, struct in_addr second, unsigned short second_port);
void client_init(struct alpha_client_relay*, struct in_addr, const unsigned short);
void client_free(void);
void client_handle_control_packet(int client_pair_id, struct in_addr sender, struct alpha_control_packet *packet, const unsigned int size);

#endif // __FILTERHOST_H_
