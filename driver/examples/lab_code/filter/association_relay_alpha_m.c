// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <assert.h>
#include <openssl/hmac.h>

#include "association_relay_alpha_m.h"
#include "../extended_math.h"
#include "../alpha_m.h"
#include "../xmalloc.h"

//! Initializes a given Alpha M association
/*!
 * Initializes a Alpha M association with a given security mode, that is how many 
 * HMACs/Nodes are transmitted per packet (a higher mode allows a higher verification 
 * rate, but causes a higher overhead).
 * 
 * @param[in,out] ass the association to be initalized (prealloced)
 * @param[in] id the id of the association (should be unique ranging from 0-255).
 * @param[in] mode always ALPHA_M
 */
void association_relay_init_alpha_m_ass(struct alpha_m_ass_relay* ass,
	const unsigned int id, const alpha_mode_t mode) {
//[[
	assert(ass != NULL);
	ass->id = id;
	ass->mode = mode;
	ass->sign_anchors = ring_buffer_new(STORE_SIGN_ANCHORS, HASHSIZE);
	ass->ack_anchors = ring_buffer_new(STORE_ACK_ANCHORS, HASHSIZE);
	// is set when S1 packets arrive
	ass->node_buf = NULL;
} //]]

//! Frees a given Alpha M Association
/*!
 * The association is freed if a not null pointer is passed. 
 * If the pointer is a null pointer no operation is performed. 
 * 	
 * @note This function should be called by association_relay_free() only
 * @param[in] ass the association to be freed
 */
void association_relay_free_alpha_m_ass(struct alpha_m_ass_relay* ass) {
//[[
	if(ass == NULL) {
		return;
	}
	if(ass->node_buf) {
		xfree(ass->node_buf);
	}
	xfree(ass);
} //]]

//! Handles an incoming Alpha M packet
/*!
 * Handles a given S1 packet. Hash chain checks are performed in 
 * association_relay_handle_s1(). This function handles only the 
 * special functions for Alpha M, i.e. caching received HMACs.
 * 	
 * @param[in,out] ass The association on that the packet came in
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the packet (including the header)
 */
void association_relay_handle_s1_alpha_m(struct alpha_m_ass_relay* ass, struct alpha_packet_s1* packet, unsigned int packet_len) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(packet_len > 0);
	struct alpha_packet_s1_m* packet_m = (struct alpha_packet_s1_m*)packet;
	ass->sec_mode = packet_m->sec_mode;
	memcpy(ass->root, (unsigned char*)packet + sizeof(alpha_packet_s1_m_t), HASHSIZE);
	ass->packets_received = 0;
	ass->packets_to_receive = packet_m->signatures_count;
	const unsigned int depth = log_2(ass->packets_to_receive);
	if(ass->packets_to_receive > 2){
		ass->node_buf = cache_tree_reallocate_node_buffer(ass->node_buf, ass->packets_to_receive, HASHSIZE);
		//store the log_2(n)-1 nodes
		unsigned char* nodes= (unsigned char*)packet + sizeof(alpha_packet_s1_m_t) + HASHSIZE;
		unsigned int i;
		for(i = 0; i < (depth - cache_tree_get_number_of_sent_signatures(ass->sec_mode)); ++i) {
			cache_tree_store_node(ass->node_buf, nodes + ((i) * HASHSIZE), 1, (i+1), ass->packets_to_receive, HASHSIZE);
		}
	}
} //]]

//! Verifies a given Alpha S2 packet
/*!
 * Verifies a given Alpha S2 packet with the content of the node cache in the given Alpha M association.
 * 	
 * @param[in] ass the association on that the packet came in
 * @param[out] anchor the anchor of the packet 
 * @param[in] packet the incoming packet
 * @param[in] packet_len the size of the incoming packet (including the header)
 * @param[out] valid if the packet is valid (checked with the given HMAC)
 */
void association_relay_verify_alpha_m(struct alpha_m_ass_relay *ass, unsigned char* anchor, unsigned char* packet, unsigned int packet_len, bool* valid) {
//[[
	assert(anchor != NULL);
	assert(ass != NULL);
	*valid = false;
	
	struct alpha_packet_s2_m* packet_m = (struct alpha_packet_s2_m*)packet ;
	const unsigned int data_index = packet_m->data_index;
	
	unsigned char* branch_nodes = packet + sizeof(alpha_packet_s2_m_t);
	const unsigned int branches_count = cache_tree_get_number_of_sent_signatures(ass->sec_mode);
	
	unsigned int real_payload_len = packet_len - (branches_count * HASHSIZE) - sizeof(struct alpha_packet_s2_m);
	unsigned char* real_payload_ptr = branch_nodes + (branches_count * HASHSIZE);
	
	const unsigned int packets = ass->packets_to_receive;
	
	unsigned char buf[log_2(packets) * HASHSIZE];
	memset(buf, 0, log_2(packets) * HASHSIZE);
	unsigned char* buf_ptr = buf;
	const unsigned int depth = log_2(packets);

	cache_tree_build_verification_buffer(ass->node_buf, branch_nodes, &(buf_ptr), data_index, packets, ass->sec_mode, HASHSIZE);

	unsigned char calc_node_buffer[log_2(packets) * HASHSIZE];
	
	if(htree_verify_branch(ass->root, HASHSIZE, buf_ptr, depth * HASHSIZE, real_payload_ptr, real_payload_len, 
		data_index, NULL, 0, alpha_m_ass_internal_leaf_gen, alpha_m_ass_internal_node_gen, NULL, 
		calc_node_buffer + HASHSIZE, calc_node_buffer) == 0) {
		*valid = true;

		cache_tree_update_buffer(ass->node_buf, data_index, packets, branch_nodes, calc_node_buffer, ass->sec_mode, HASHSIZE);

		ass->packets_received++;
		if(ass->packets_received >= ass->packets_to_receive) {
			association_relay_set_sign_anchor((struct association_relay*)ass, ((alpha_packet_s2_t *) packet)->anchor);
		}

	} else {
		*valid = false;

		//update buffer if possible
		if(ass->sec_mode == CACHE_TREE_TWO_SIGNATURES_PER_PACKET) {
			
			if(cache_tree_verify_new_cache_entry(ass->node_buf, branch_nodes, branch_nodes + HASHSIZE, data_index, 	packets,
				alpha_m_ass_internal_node_gen, 	NULL, HASHSIZE)) {	
											
				cache_tree_update_buffer(ass->node_buf, data_index, packets, branch_nodes, calc_node_buffer, ass->sec_mode, HASHSIZE);
				
			}
		}
	}

} //]]
