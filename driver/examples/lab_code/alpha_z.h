// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef ALPHA_Z_H__
#define ALPHA_Z_H__

#include "association.h"

typedef struct alpha_z_ass{
	//! Pointer to the basic client information (such as address)
	unsigned int client_id;
	//! identifier
	unsigned int id;
	//! if the association allows sending/receiving or both
	association_direction_t direction;
	//! Timestamp for handling timeouts for this association
	time_t timestamp;
	//! the mode this association has
	alpha_mode_t mode;
	//! State of the association
	association_state_t state[ASS_TRANS_MODE_COUNT];
	//! Packet queue
	list_t* packet_queue;
	//! Hash chain element for signing (from the client)
	struct ring_buffer* sign_anchors;
	//! Hash chain element for acking (from the client)
	struct ring_buffer* ack_anchors;
	//! hash chain for signing (this is our hash chain)
	hash_chain_t* sign_hash_chain;
	//! hash chain for acking (this is our hash chain)
	hash_chain_t* ack_hash_chain;
	//! the anchor which is sent within the next a1 packet as (sign) return anchor
	/*!
	 * Note that the sign anchors ringbuffer stores multiple hashes and it is not always
	 * the case that the next anchor is the return anchor
	 */
	unsigned char ret_anchor[HASHSIZE];
} alpha_z_ass_t;

unsigned int alpha_z_ass_pop_packet(alpha_z_ass_t*, list_t*);

void alpha_z_ass_flush_queue(alpha_z_ass_t*, list_t*);

void alpha_z_ass_collect_timeout(const alpha_z_ass_t*, struct timeval*);

ap_err_code alpha_z_ass_init(alpha_z_ass_t*, const config_t*, const uint32_t, const uint32_t, const association_direction_t);

ap_err_code alpha_z_ass_free(alpha_z_ass_t*);

socklen_t alpha_z_ass_send_s2(const config_t*, alpha_z_ass_t*);

ap_err_code alpha_z_ass_handle_s2(alpha_z_ass_t* const, const alpha_packet_s2_t* const, unsigned char* const,
	const size_t, bool* const, unsigned char**, size_t* const);

#endif /*ALPHA_Z_H__*/
