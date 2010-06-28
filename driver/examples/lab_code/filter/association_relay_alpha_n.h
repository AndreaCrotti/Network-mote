// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __ASS_RELAY_ALPHA_N_H__
#define __ASS_RELAY_ALPHA_N_H__

#include "association_relay.h"

//! Represents a Alpha N association
/*!
 * @note SYNC with alpha_ass_relay
 */
struct alpha_n_ass_relay {
	//! unique identifier for the association (unique with regards to all association relays)
	unsigned int id;
	//! the mode of the association (always ALPHA_N)
	unsigned int mode;
	//! buffer in that sign anchors are stored
	struct ring_buffer* sign_anchors;
	//! buffer in that ack anchors are stored
	struct ring_buffer* ack_anchors;
	//! the most recent HMAC that is needed for content verification (i.e. message integrity)
	unsigned char hmac[HASHSIZE];
};

void association_relay_init_alpha_n_ass(struct alpha_n_ass_relay*, const unsigned int, const alpha_mode_t);

void association_relay_free_alpha_n_ass(struct alpha_n_ass_relay*);

void association_relay_handle_s1_alpha_n(struct alpha_n_ass_relay*, struct alpha_packet_s1*, unsigned int);

void association_relay_verify_alpha_n(struct alpha_n_ass_relay*, unsigned char*, unsigned char*, unsigned int, bool*);

#endif // __ASS_RELAY_ALPHA_N_H__
