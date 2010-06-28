// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <assert.h>

#include "association_relay_alpha_c.h"
#include "../packet.h"
#include "../xmalloc.h"
#include "../digest.h"

//! Initializes a preallocated Alpha C association
/*!
 * Initializes a already allocated Alpha C association with default values 
 * and creates necessary default structures. 
 *	
 * @param[in,out] ass preallocated alpha c association relay (pre allocated)
 * @param[in] id the identifier of this association (should be unique as this is the position in the relay array)
 */
void association_relay_init_alpha_c_ass(struct alpha_c_ass_relay* ass, const unsigned int id, const alpha_mode_t mode) {
//[[
	assert(ass != NULL);
	ass->id = id;
	ass->mode = ALPHA_C;
	ass->sign_anchors = ring_buffer_new(STORE_SIGN_ANCHORS, HASHSIZE);
	ass->ack_anchors = ring_buffer_new(STORE_ACK_ANCHORS, HASHSIZE);
	ass->hmacs = ring_buffer_new(ALPHA_C_RING_BUFSIZE, HASHSIZE);
} //]]

//! Frees a given ALpha C Association
/*!
 * The association is freed if a not null pointer is passed. 
 * If the pointer is a null pointer no operation is performed. 
 * 	
 * @note This function should be called by association_relay_free() only
 * @param[in] ass the association to be freed
 */
void association_relay_free_alpha_c_ass(struct alpha_c_ass_relay* ass) {
//[[
	if(ass == NULL) {
		return;
	}

	ring_buffer_free(ass->hmacs);
	xfree(ass);
} //]]

//! Handles an incoming Alpha C packet
/*!
 * Handles a given S1 packet. Hash chain checks are performed in 
 * association_relay_handle_s1(). This function handles only the 
 * special functions for Alpha C, i.e. buffering received HMACs.
 * 	
 * @param[in,out] ass The association on that the packet came in
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the packet (including the header)
 */
void association_relay_handle_s1_alpha_c(struct alpha_c_ass_relay* ass, struct alpha_packet_s1* packet, unsigned int packet_len) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(packet_len > 0);
	assert(packet_len >= sizeof(struct alpha_packet_s1) + packet->signatures_count * HASHSIZE);

	// buffer the hmacs
	unsigned int i;
	unsigned char* hmac_ptr = (unsigned char*)packet + sizeof(struct alpha_packet_s1);
	for(i = 0; i < packet->signatures_count; ++i) {
		ring_buffer_insert(ass->hmacs, hmac_ptr);
		hmac_ptr += HASHSIZE;
	}

	ass->packets_to_receive = packet->signatures_count;
	ass->packets_received = 0;
} //]]

//! Verifies a given Alpha S2 packet
/*!
 * Verifies a given Alpha S2 packet with the content of the buffer in the given Alpha C association.
 * 	
 * @param[in] ass the association on that the packet came in
 * @param[out] anchor the anchor of the packet 
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the incoming packet (including the header)
 * @param[out] valid if the packet is valid (checked with the given HMAC)
 */
void association_relay_verify_alpha_c(struct alpha_c_ass_relay *ass, unsigned char* anchor, unsigned char* packet, unsigned int packet_len, bool* valid) {
//[[
	assert(anchor != NULL);
	assert(ass != NULL);
	*valid = false;
	
	unsigned char computed_hmac[HASHSIZE];
	hmac(packet + sizeof(struct alpha_packet_s2), packet_len - sizeof(struct alpha_packet_s2), anchor, HASHSIZE, computed_hmac);
	*valid = ring_buffer_find_and_move(ass->hmacs, computed_hmac);
	
	if(*valid) {
		ass->packets_received++;
		if(ass->packets_to_receive <= ass->packets_received) {
			association_relay_set_sign_anchor((struct association_relay*)ass, ((alpha_packet_s2_t *) packet)->anchor);
		}
	}
} //]]
