// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __ASS_RELAY_ALPHA_M_H__
#define __ASS_RELAY_ALPHA_M_H__

#include "association_relay.h"

//! Represents a Alpha M association
/*!
 * @note SYNC with alpha_ass_relay
 */
struct alpha_m_ass_relay {
	//! unique identifier (for all association relays)
	unsigned int id;
	//! the mode for this association (always ALPHA_M)
	unsigned int mode;
	//! contains a buffer in that sign anchors are stored
	struct ring_buffer* sign_anchors;
	//! contains a buffer in that ack anchors are stored
	struct ring_buffer* ack_anchors;
	//! buffer for received nodes (is always (2 * log(n)) - 1 with n = number of leafs)
	unsigned char* node_buf;
	//! the root of the tree (that is the received root when receiving data)
	unsigned char root[HASHSIZE];
	//! number of packets that need to be received, stays constant during a single transmission (can also be not of the form 2^x)
	unsigned int packets_to_receive;
	//! the security mode of the association (that is the number of nodes in each data packet)
	unsigned int sec_mode;
	//! this is the number of packets that have already been received
	unsigned int packets_received; 
};

void association_relay_init_alpha_m_ass(struct alpha_m_ass_relay*, const unsigned int, const alpha_mode_t);

void association_relay_free_alpha_m_ass(struct alpha_m_ass_relay*);

void association_relay_handle_s1_alpha_m(struct alpha_m_ass_relay*, struct alpha_packet_s1*, unsigned int);

void association_relay_verify_alpha_m(struct alpha_m_ass_relay*, unsigned char*, unsigned char*, unsigned int, bool*);

#endif // __ASS_RELAY_ALPHA_M_H__
