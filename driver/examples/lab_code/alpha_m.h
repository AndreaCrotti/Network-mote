// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file alpha_m.h This file implements the Alpha M mode
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * @date April 2009
 */

#ifndef __ALPHA_M_H_
#define __ALPHA_M_H_

#include "host.h"
#include "cache_tree.h"

#define ALPHA_M_MINIMUM_PACKETS 2
#define ALPHA_M_PACKET_TIMEOUT 0
#define ALPHA_M_RANDOM_PACKET_SIZE 1

//! Additional data for each host
/*!
 * This struct/typedef contains additional data
 * required for the alpha m mode.
 *
 * @see host.h
 */

/* TODO: Christian: Check processsor alignment */
typedef struct alpha_m_ass{
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
	//! tree to transmit data
	hash_tree_t* tree;
	//! root node received
	unsigned char root[HASHSIZE];
	//! Number of S2 packets which need to be sent
	size_t packets_to_send;
	//! Number of packets which need to be received
	size_t packets_to_receive;
	//! The number of S2 packets already authenticated
	size_t packets_received;
	//! Buffers nodes from the tree
	unsigned char* node_buffer;
	//! The security mode used for this association
	cache_tree_security_mode_t sec_mode;
} alpha_m_ass_t;

unsigned int alpha_m_ass_pop_packet(alpha_m_ass_t*, list_t*);

void alpha_m_ass_flush_queue(alpha_m_ass_t*, list_t*);

void alpha_m_ass_collect_timeout(const alpha_m_ass_t*, struct timeval*);

ap_err_code alpha_m_ass_init(alpha_m_ass_t*, const config_t*, const uint32_t, const uint32_t, const association_direction_t, const cache_tree_security_mode_t);

ap_err_code alpha_m_ass_free(alpha_m_ass_t*);

socklen_t alpha_m_ass_send_s1(const config_t*, alpha_m_ass_t*);

ap_err_code alpha_m_ass_handle_s1(alpha_m_ass_t*, const alpha_packet_s1_t*, const unsigned char*, const size_t);

socklen_t alpha_m_ass_send_s2(const config_t*, alpha_m_ass_t*);

ap_err_code alpha_m_ass_handle_s2(alpha_m_ass_t* const, const alpha_packet_s2_t* const, unsigned char* const,
	const size_t, bool* const, unsigned char**, size_t* const);

int alpha_m_ass_internal_leaf_gen(const unsigned char*, int, unsigned char*, int, unsigned char*, htree_gen_args_t*);

int alpha_m_ass_internal_node_gen(const unsigned char*, const unsigned char*, int, unsigned char*, htree_gen_args_t*);

#endif // __ALPHA_M_H_
