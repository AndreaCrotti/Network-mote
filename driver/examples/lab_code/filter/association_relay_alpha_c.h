// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __ASS_RELAY_ALPHA_C_H__
#define __ASS_RELAY_ALPHA_C_H__

#include "association_relay.h"

//! Alpha C association relay
/*!
 * The Alpha association relay is a special of a association relay 
 *	
 * @see association_relay
 * @remark Do not modify the fields that are also contains in the association_relay or synch with all other modes
 * @note SYNC with alpha_ass_relay
 */
struct alpha_c_ass_relay {
	//! identifier
	unsigned int id;
	//! the alpha mode for this association relay (that is always ALPHA_C in this case)
	unsigned int mode;
	//! Buffer containing sign anchors for authentication (hash chain elements)
	struct ring_buffer* sign_anchors;
	//! Buffer containing ack anchors for authentication (hash chain elements)
	struct ring_buffer* ack_anchors;
	//! Buffer containing received hmacs
	struct ring_buffer* hmacs;
	//! Number of packets that need to be received. this is set after a S1 packet arrives
	unsigned int packets_to_receive;
	//! Counts the number of received s2 packet to change the state if necessary
	unsigned int packets_received;
};

void association_relay_init_alpha_c_ass(struct alpha_c_ass_relay*, const unsigned int, const alpha_mode_t);

void association_relay_free_alpha_c_ass(struct alpha_c_ass_relay*);

void association_relay_handle_s1_alpha_c(struct alpha_c_ass_relay*, struct alpha_packet_s1*, unsigned int);

void association_relay_verify_alpha_c(struct alpha_c_ass_relay*, unsigned char*, unsigned char*, unsigned int, bool*);

#endif // __ASS_RELAY_ALPHA_C_H__
