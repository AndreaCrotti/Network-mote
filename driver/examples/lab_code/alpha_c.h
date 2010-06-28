// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * 	@file alpha_c.h Implementation of Alpha C
 * 	@author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * 	@date April 2009
 *
 */

#ifndef __ALPHA_C_H_
#define __ALPHA_C_H_

#include "association.h"
#include "ring_buffer.h"

#define ALPHA_C_MINIMUM_PACKETS 0
#define ALPHA_C_PACKET_TIMEOUT 0

//! Additional data for each host
/*!
 * This struct/typedef contains additional data
 * required for the alpha c mode.
 * @see host.h
 */
typedef struct alpha_c_ass {

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
	//! Number of packets have been signed by the last S1 packet (important for server and client)
	/*!
	 * On the server (sender) side this value shows how many s2 packets need to be sent. On the client
	 * (receiver) side this shows how many s2 packets need to be received.
	 */
	uint32_t packets_pre_signed;
	//! Ringbuffer for storing pre signatures
	/*unsigned char signatures[ALPHA_C_RING_BUFSIZE * HASHSIZE];
	//! Pointer to the location of next insertion in the ring buffer
	unsigned char* signatures_write_ptr;
	//! Pointer from which data is valid in the ring buffer (from read to write)
	unsigned char* signatures_read_ptr;
	//! Pointer, pointing to the end of the ringbuffer
	unsigned char* signatures_end_ptr;*/
	//! timestamp, so that packets can be buffered over a given time
	/*!
	 * This timestamp is refreshed whenever all S2 packets are send out
	 */
	 ring_buffer_t* signatures;
	 time_t buffer_time;
} alpha_c_ass_t;

unsigned int alpha_c_ass_pop_packet(alpha_c_ass_t* ass, list_t* queue);

void alpha_c_ass_flush_queue(alpha_c_ass_t*, list_t*);

void alpha_c_ass_collect_timeout(const alpha_c_ass_t*, struct timeval*);

ap_err_code alpha_c_ass_init(alpha_c_ass_t*, const config_t*, const uint32_t, const uint32_t, const association_direction_t);

ap_err_code alpha_c_ass_free(alpha_c_ass_t*);

socklen_t alpha_c_ass_send_s1(const config_t*, alpha_c_ass_t*);

ap_err_code alpha_c_ass_handle_s1(alpha_c_ass_t* ass, const alpha_packet_s1_t* packet,
	const unsigned char* payload, const size_t payload_size);

socklen_t alpha_c_ass_send_s2(const config_t*, alpha_c_ass_t*);

ap_err_code alpha_c_ass_handle_s2(alpha_c_ass_t* const, const alpha_packet_s2_t* const, unsigned char* const,
	const size_t, bool* const, unsigned char**, size_t* const);

#endif // __ALPHA_C_H_
