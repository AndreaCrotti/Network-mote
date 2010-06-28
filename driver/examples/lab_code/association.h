// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */



#ifndef __ASS_H_
#define __ASS_H_

#include "host.h"

#define ASS_MAX_NUMBER 255

//! Association states
/*!
 * 	For each association there are three modes (HANDSHAKE, SENDING, RECEIVING)
 * 	and for each mode there is a state.
 *
 * 	\see association_transmission_mode
 */
typedef enum association_state{
	//! client was just created, no handshake done yet
	ASS_STATE_NEW = 0,
	//! client initiated the handshake, sent SYN and waits for ack
	ASS_STATE_SENT_SYN_WAIT_ACK = 1,
	//! other side initiated the handshake, we got SYN, sent ACK and wait for ACKACK
	ASS_STATE_SENT_ACK_WAIT_ACKACK = 2,
	//! handshake is done, packets can now be transmitted
	ASS_STATE_READY = 3,
	//! we just sent the first packet of the signature scheme (if we want to send something to the remote host)
	ASS_STATE_SENT_S1_WAIT_A1 = 4,
	//! we just sent the second packet of the signature scheme (if the other side wants to send something to US)
	ASS_STATE_SENT_A1_WAIT_S2 = 5,
	//! we just sent the CONNECT and are waiting for the return
	ASS_STATE_SENT_CONNECT_WAIT_RETURN_CONNECT = 6
} association_state_t;

//! Update this every time you add new states
#define ASS_STATE_COUNT 7

//! mode in which the current association is
/*!
 * 	\see association_state
 */
typedef enum association_transmission_mode{
	//! we are currently doing a handshake
	ASS_TRANS_MODE_HANDSHAKE = 0,
	//! we are sending some data
	ASS_TRANS_MODE_SENDING = 1,
	//! we wait to receive data
	ASS_TRANS_MODE_RECEIVING = 2
} association_transmission_mode_t;

//! Update this every time you add new transmission modes
#define ASS_TRANS_MODE_COUNT  3

//! Specifies in which direction the association can sent data
typedef enum association_direction{
	//! Bidirectional (only used for the default association!)
	ASS_DIRECTION_BIDIRECTIONAL = 0,
	//! This association only allows sending data
	ASS_DIRECTION_OUTGOING = 1,
	//! This association only allows receiving data
	ASS_DIRECTION_INCOMING = 2
} association_direction_t;


//! Very basic alpha association structure
/*!
 * 	This structure should be expanded by each alpha mode. Please
 *  keep the field also in this order, so that a specific alpha mode
 *  association can be casted on this one.
 */
typedef struct association{
	//! Pointer to the basic client information (such as address)
	unsigned int client_id;
	//! identifier
	unsigned int id;
	//! if the association allows sending/receiving or both
	association_direction_t direction;
	//! Timestamp for handling timeouts for this association (S1 timeouts)
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
		Note that the sign anchors ringbuffer stores multiple hashes and it is not always
		the case that the next anchor is the return anchor
	 */
	unsigned char ret_anchor[HASHSIZE];
} association_t;

bool association_verify_anchor(const unsigned char*, const unsigned char*);

ap_err_code association_init(association_t*, const config_t*, const uint32_t, const unsigned int, const association_direction_t);

ap_err_code association_free(association_t*);

size_t association_get_queue_size(const association_t*);

ap_err_code association_move_packet_queue(association_t*, association_t*);

ap_err_code association_add_packet(association_t*, unsigned char*, const size_t);

socklen_t association_send_s1(association_t*, const config_t*);

socklen_t association_send_a1(association_t*, const config_t*);

socklen_t association_send_s2(association_t*, const config_t*);

ap_err_code association_handle_s1(association_t*, const config_t*, const alpha_packet_s1_t*, const size_t, bool*);

ap_err_code association_handle_s2(association_t*, const config_t*, const alpha_packet_s2_t*, unsigned char*, const size_t, bool* const, unsigned char**, size_t* const);

ap_err_code association_handle_a1(association_t*, const config_t*, const alpha_packet_a1_t*, bool*);

ap_err_code association_init_new_chains(association_t*, const config_t*, const association_direction_t);

unsigned char* association_hash_hmac_packet(const association_t*, const unsigned char*, const size_t, unsigned char*, unsigned char*);

size_t association_remaining_anchors_sign(const association_t*);

size_t association_remaining_anchors_ack(const association_t*);

void association_dec_ack_rounds(association_t*);

void association_dec_sign_rounds(association_t*);

ap_err_code association_handle_timeouts(association_t*, const config_t*);

void association_get_packet_timeout(association_t*, struct timeval*);

unsigned int ass_pop_packets(association_t*, list_t*);

void association_flush_queue(association_t*, list_t*);

// Simple accessors
unsigned char* association_get_sign_element(const association_t*);

unsigned char* association_get_ack_element(const association_t*);

association_state_t association_get_state(const association_t*, const association_transmission_mode_t);

ap_err_code association_set_state(association_t*, const association_transmission_mode_t, const association_state_t);

alpha_mode_t association_get_mode(const association_t*);

uint32_t association_get_id(const association_t*);

uint32_t association_get_client_id(const association_t*);

time_t association_get_timestamp(const association_t*);

void association_set_timestamp(association_t*, const time_t);

ap_err_code association_add_sign_element(association_t*, const unsigned char*);

ap_err_code association_add_ack_element(association_t*, const unsigned char*);


#endif // __ASS_H_
