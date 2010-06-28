// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <assert.h>

#include "association_relay_alpha_n.h"
#include "../xmalloc.h"
#include "../digest.h"

//! Initializes a given Alpha N association
/*!
 * Initializes a Alpha N association
 * 
 * @param[in,out] ass the association to be initalized (prealloced)
 * @param[in] id the id of the association (should be unique ranging from 0-255).
 * @param[in] mode always ALPHA_N
 */
void association_relay_init_alpha_n_ass(struct alpha_n_ass_relay* ass, const unsigned int id, const alpha_mode_t mode) {
//[[
	assert(ass != NULL);
	ass->id = id;
	ass->mode = mode;
	ass->sign_anchors = ring_buffer_new(STORE_SIGN_ANCHORS, HASHSIZE);
	ass->ack_anchors = ring_buffer_new(STORE_ACK_ANCHORS, HASHSIZE);
} //]]

//! Frees a given ALpha N Association
/*!
 * The association is freed if a not null pointer is passed. 
 * If the pointer is a null pointer no operation is performed. 
 * 	
 * @note This function should be called by association_relay_free() only	
 * @param[in] ass the association to be freed
*/
void association_relay_free_alpha_n_ass(struct alpha_n_ass_relay* ass) {
//[[
	if(ass == NULL) {
		return;
	}

	xfree(ass);
} //]]

//! Handles an incoming Alpha N packet
/*!
 * Handles a given S1 packet. Hash chain checks are performed in 
 * association_relay_handle_s1(). This function handles only the 
 * special functions for Alpha n, i.e. buffering received HMACs.
 * 	
 * @param[in,out] ass The association on that the packet came in
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the packet (including the header)
 */
void association_relay_handle_s1_alpha_n(struct alpha_n_ass_relay* ass, struct alpha_packet_s1* packet, unsigned int packet_len) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(packet_len > 0);

	//just store the hmac
	memcpy(ass->hmac, (unsigned char*)packet + sizeof(struct alpha_packet_s1), HASHSIZE);
} //]]

//! Verifies a given Alpha S2 packet
/*!
 * Verifies a given Alpha S2 packet with the stored HMAC in the given Alpha N association.
 * 	
 * @param[in] ass the association on that the packet came in
 * @param[out] anchor the anchor of the packet 
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the incoming packet (including the header)
 * @param[out] valid if the packet is valid (checked with the given HMAC)
 */
void association_relay_verify_alpha_n(struct alpha_n_ass_relay *ass, unsigned char* anchor, unsigned char* packet, unsigned int packet_len, bool* valid) {
//[[
	assert(anchor != NULL);
	assert(ass != NULL);
	*valid = false;

	unsigned char computed_hmac[HASHSIZE];
	hmac(packet + sizeof(struct alpha_packet_s2), packet_len - sizeof(struct alpha_packet_s2), anchor, HASHSIZE, computed_hmac);
	*valid = (memcmp(ass->hmac, computed_hmac, HASHSIZE) == 0);
	if(*valid) {
		association_relay_set_sign_anchor((struct association_relay*)ass, ((struct alpha_packet_s2*)packet)->anchor);
	}
} //]]
