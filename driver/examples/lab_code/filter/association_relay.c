// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <assert.h>
#include <inttypes.h>

#include "association_relay.h"
#include "client.h"

#include "../xmalloc.h"
#include "../extended_math.h"
#include "../cache_tree.h"
#include "../digest.h"
#include "../control_association.h"
#include "../alpha_m.h"

#include "association_relay_alpha_n.h"
#include "association_relay_alpha_c.h"
#include "association_relay_alpha_m.h"
#include "association_relay_alpha_z.h"

//! Frees a given association relay
/*!
 * Depending on the association relay, the free method is choosen accordingly.
 * @param[in,out] relay the relay to be freed or NULL
 */
void association_relay_free(struct association_relay* relay) {
//[[

	if(relay == NULL) {
		return;
	}

	ring_buffer_free(relay->sign_anchors);
	ring_buffer_free(relay->ack_anchors);

	switch(relay->mode) {
		case ALPHA_N:
			association_relay_free_alpha_n_ass((struct alpha_n_ass_relay*)relay);
			break;
		case ALPHA_M:
			association_relay_free_alpha_m_ass((struct alpha_m_ass_relay*)relay);
			break;
		case ALPHA_C:
			association_relay_free_alpha_c_ass((struct alpha_c_ass_relay*)relay);
			break;
		case ALPHA_Z:
			association_relay_free_alpha_z_ass((struct alpha_z_ass_relay*)relay);
			break;
	}

} //]]

//! Handles a S1 packet for a given association relay
/*!
 * Depending on the association relay, the method is choosen accordingly.
 * 
 * @param[in,out] relay the relay on that the packet came in
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the packet
 */
void association_relay_handle_s1(struct association_relay* ass, struct alpha_packet_s1* packet, unsigned int packet_len) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(packet_len > 0);

	switch(ass->mode) {
		case ALPHA_N:
			association_relay_handle_s1_alpha_n((struct alpha_n_ass_relay*)ass, packet, packet_len);
			break;
		case ALPHA_M:
			association_relay_handle_s1_alpha_m((struct alpha_m_ass_relay*)ass, packet, packet_len);
			break;
		case ALPHA_C:
			association_relay_handle_s1_alpha_c((struct alpha_c_ass_relay*)ass, packet, packet_len);
			break;
	}

} //]]

//! Handles a S2 packet for a given association relay
/*!
 * Depending on the association relay, the method is choosen accordingly.
 * 	
 * @param[in,out] relay the relay on that the packet came in
 * @param[out] anchor space in that the anchor is put
 * @param[in] packet the packet to be verified (including the header)
 * @param[in] packet_len the size of the packet
 * @param[out] valid whether the packet is valid or not
 */
void association_relay_verify(struct association_relay *ass, unsigned char* anchor, unsigned char* packet, unsigned int packet_len, bool* valid) {
//[[
	assert(anchor != NULL);
	assert(ass != NULL);
	assert(packet != NULL);
	assert(packet_len > 0);
	assert(valid != NULL);

	*valid = false;

	switch(ass->mode) {
		case ALPHA_N:
			association_relay_verify_alpha_n((struct alpha_n_ass_relay*)ass, anchor, packet, packet_len, valid);
			break;
		case ALPHA_M:
			association_relay_verify_alpha_m((struct alpha_m_ass_relay*)ass, anchor, packet, packet_len, valid);
			break;
		case ALPHA_C:
			association_relay_verify_alpha_c((struct alpha_c_ass_relay*)ass, anchor, packet, packet_len, valid);
			break;
	}
} //]]

//! Handles a incoming request for a new association
/*!
 * This method is called whenever a packet that is encapsulated in a Alpha S2 packet comes in and the control packet 
 * is of the new associations.
 * 	
 * @param[in] client_pair_id the identifier for the both clients
 * @param[in] sender address of the sender (in the client pair)
 * @param[in] packet the incoming packet (with control-, but not Alpha header)
 * @param[in] packet_len the size of the incoming packet (with control-, but not Alpha header)
 */
void association_relay_handle_new_associations(unsigned int client_pair_id, struct in_addr sender, const struct alpha_packet_new_association* packet, const unsigned int packet_len) {
//[[
	assert(packet != NULL);
	assert(packet_len > 0);

	// we got new associations
	const size_t header_size = sizeof(struct alpha_packet_new_association);
	uint8_t mode;
	uint8_t new_id;
	struct association_relay* relay = NULL;
	unsigned char sign_anchor[HASHSIZE];
	unsigned char ack_anchor[HASHSIZE];
	unsigned int i;
	for(i = 0; i < packet->num_associations; ++i) {
		ca_decode_ass_metadata(i, (const unsigned char*)packet, header_size, &new_id, &mode);
		if(client_find_association(client_pair_id, sender, new_id)) {
			continue;
		}
		switch(mode) {
			case ALPHA_M:
				relay = xmalloc(sizeof(struct alpha_m_ass_relay));
				association_relay_init_alpha_m_ass((struct alpha_m_ass_relay*)relay, new_id, mode);
				break;
			case ALPHA_N:
				relay = xmalloc(sizeof(struct alpha_n_ass_relay));
				association_relay_init_alpha_n_ass((struct alpha_n_ass_relay*)relay, new_id, mode);
				break;
			case ALPHA_C:
				relay = xmalloc(sizeof(struct alpha_c_ass_relay));
				association_relay_init_alpha_c_ass((struct alpha_c_ass_relay*)relay, new_id, mode);
				break;
			case ALPHA_Z:
				relay = xmalloc(sizeof(struct alpha_z_ass_relay));
				association_relay_init_alpha_z_ass((struct alpha_z_ass_relay*)relay, new_id, mode);
				break;
		}

		ca_decode_ass(i, (const unsigned char*)packet, header_size, sign_anchor, ack_anchor, HASHSIZE);

		association_relay_set_sign_anchor(relay, sign_anchor);
		association_relay_set_ack_anchor(relay, ack_anchor);
		client_add_association(client_pair_id, sender, relay);
	}
} //]]

//! Handles a incoming acknowledgement for a new association
/*!
 * This method is called whenever a packet that is encapsulated in a Alpha S2 packet comes in and the control packet 
 * is of the acknowledgement.
 * 	
 * @param[in] client_pair_id the identifier for the both clients
 * @param[in] sender address of the sender (in the client pair)
 * @param[in] packet the incoming packet (with control-, but not Alpha header)
 * @param[in] packet_len the size of the incoming packet (with control-, but not Alpha header)
 */
void association_relay_handle_new_associations_ack(unsigned int client_pair_id, struct in_addr sender, const struct alpha_packet_new_ass_ack* packet, const unsigned int packet_len) {
//[[
	assert(packet != NULL);
	assert(packet_len > 0);
	const size_t header_size = sizeof(struct alpha_packet_new_ass_ack);
	uint8_t mode;
	uint8_t new_id;
	struct association_relay* relay = NULL;
	unsigned char sign_anchor[HASHSIZE];
	unsigned char ack_anchor[HASHSIZE];
	unsigned int i;
	for(i = 0; i < packet->num_associations; ++i) {
		ca_decode_ass_metadata(i, (const unsigned char*)packet, header_size, &new_id, &mode);
		if(client_find_association(client_pair_id, sender, new_id)) {
			continue;
		}
		switch(mode) {
			case ALPHA_M:
				relay = xmalloc(sizeof(struct alpha_m_ass_relay));
				association_relay_init_alpha_m_ass((struct alpha_m_ass_relay*)relay, new_id, mode);
				break;
			case ALPHA_N:
				relay = xmalloc(sizeof(struct alpha_n_ass_relay));
				association_relay_init_alpha_n_ass((struct alpha_n_ass_relay*)relay, new_id, mode);
				break;
			case ALPHA_C:
				relay = xmalloc(sizeof(struct alpha_c_ass_relay));
				association_relay_init_alpha_c_ass((struct alpha_c_ass_relay*)relay, new_id, mode);
				break;
			case ALPHA_Z:
				relay = xmalloc(sizeof(struct alpha_z_ass_relay));
				association_relay_init_alpha_z_ass((struct alpha_z_ass_relay*)relay, new_id, mode);
				break;
		}

		ca_decode_ass(i, (const unsigned char*)packet, header_size, sign_anchor, ack_anchor, HASHSIZE);

		association_relay_set_sign_anchor(relay, sign_anchor);
		association_relay_set_ack_anchor(relay, ack_anchor);
		client_add_association(client_pair_id, sender, relay);
	}
} //]]


////// SELECTORS

// TODO: document me
void association_relay_set_sign_anchor(struct association_relay *ass, unsigned char *anchor) {
//[[
	assert(ass != NULL);
	assert(anchor != NULL);
	//memcpy(ass->sign_anchor, anchor, HASHSIZE);
	ring_buffer_insert(ass->sign_anchors, anchor);
} //]]

// TODO: document me
void association_relay_set_ack_anchor(struct association_relay *ass, unsigned char *anchor) {
//[[
	assert(ass != NULL);
	assert(anchor != NULL);
	//memcpy(ass->ack_anchor, anchor, HASHSIZE);
	ring_buffer_insert(ass->ack_anchors, anchor);
} //]]

// TODO: document me
unsigned char* association_relay_get_sign_anchor(struct association_relay *ass) {
//[[
	assert(ass != NULL);
	return ring_buffer_const_read(ass->sign_anchors);
} //]]

// TODO: document me
unsigned char* association_relay_get_ack_anchor(struct association_relay *ass) {
//[[
	assert(ass != NULL);
	return ring_buffer_const_read(ass->ack_anchors);
} //]]

/** Compare the digest of a newly received anchor to the previously stored anchor
 * @param	id	the client pairs id
 * @param	ass_id	association id
 * @param	addr	the relay_clients addr
 * @param	anchor	pointer to the received anchor
 * @return	0 upon success, !=0 otherwise
 */
int association_relay_verify_sign_anchor(struct association_relay *ass, unsigned char *anchor) {
//[[
	assert(anchor != NULL);
	assert(ass != NULL);

	unsigned char anchor_digest[HASHSIZE];
	create_digest(anchor, HASHSIZE, anchor_digest);
	if(ring_buffer_find_and_move(ass->sign_anchors, anchor_digest)) {
		return 0;
	} else {
		unsigned char double_hashed[HASHSIZE];
		create_digest(anchor_digest, HASHSIZE, double_hashed);
		return (ring_buffer_find_and_move(ass->sign_anchors, double_hashed)) ? 0 : -1;
	}
 } //]]

 /** Compare the digest of a newly received anchor to the previously stored anchor
 * @param	id	the client pairs id
 * @param	ass_id	association id
 * @param	addr	the relay_clients addr
 * @param	anchor	pointer to the received anchor
 * @return	0 upon success, !=0 otherwise
 */
int association_relay_verify_ack_anchor(struct association_relay *ass, unsigned char *anchor) {
//[[
	assert(anchor != NULL);
	assert(ass != NULL);

	unsigned char anchor_digest[HASHSIZE];
	create_digest(anchor, HASHSIZE, anchor_digest);
	if(ring_buffer_find_and_move(ass->ack_anchors, anchor_digest)) {
		return 0;
	} else {
		unsigned char double_hashed[HASHSIZE];
		create_digest(anchor_digest, HASHSIZE, double_hashed);
		return (ring_buffer_find_and_move(ass->ack_anchors, double_hashed)) ? 0 : -1;
	}
 } //]]
