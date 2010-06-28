// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef ASS_RELAY_H__
#define ASS_RELAY_H__

#include <stdbool.h>
#include <string.h>

#include "../ring_buffer.h"
#include "../association.h"
#include "association_relay_alpha_n.h"
#include "association_relay_alpha_c.h"
#include "association_relay_alpha_m.h"

#include "defines.h"

#define STORE_SIGN_ANCHORS 10
#define STORE_ACK_ANCHORS STORE_SIGN_ANCHORS

// This represents one single association in an alpha connection
struct association_relay {

	unsigned int id;

	// Which alpha mode is this association running on?
	unsigned int mode;

	// Hash chain anchors (SIGN and ACK) and last seen HMAC
	//unsigned char sign_anchor[HASHSIZE];
	//unsigned char ack_anchor[HASHSIZE];
	struct ring_buffer* sign_anchors;
	struct ring_buffer* ack_anchors;

};

void association_relay_free(struct association_relay*);

void association_relay_handle_s1(struct association_relay*, struct alpha_packet_s1*, unsigned int);

void association_relay_verify(struct association_relay*, unsigned char*, unsigned char*, unsigned int, bool*);

void association_relay_handle_new_associations(unsigned int, struct in_addr sender, const struct alpha_packet_new_association*, const unsigned int);

void association_relay_handle_new_associations_ack(unsigned int, struct in_addr sender, const struct alpha_packet_new_ass_ack*, const unsigned int);

void association_relay_set_sign_anchor(struct association_relay *ass, unsigned char *anchor);

void association_relay_set_ack_anchor(struct association_relay *ass, unsigned char *anchor);

unsigned char* association_relay_get_sign_anchor(struct association_relay *ass);

unsigned char* association_relay_get_ack_anchor(struct association_relay *ass);

int association_relay_verify_sign_anchor(struct association_relay *ass, unsigned char *anchor);

int association_relay_verify_ack_anchor(struct association_relay *ass, unsigned char *anchor);

#endif /*ASS_RELAY_H__*/
