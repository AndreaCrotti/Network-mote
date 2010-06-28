// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	alpha_m.c
 * @brief	ALPHA-M related functions
 */

#include <assert.h>

#include "alpha_m.h"
#include "extended_math.h"
#include "digest.h"
#include "tools.h"
#include "xmalloc.h"
#include "hashtree.h"
#include "cache_tree.h"

//! Returns the time, which the association should wait before processing new packets
/*!
 * @param[in] ass not used yet
 * @param[out] timeout the timeout (preallocated struct please)
 * @return The time which the association should wait
 */
void alpha_m_ass_collect_timeout(const alpha_m_ass_t* ass, struct timeval* timeout) {
//[[
	assert(ass != NULL);
	assert(timeout != NULL);
	if(ALPHA_M_PACKET_TIMEOUT == 0) {
		timeout->tv_sec =  0;
		timeout->tv_usec = 0;
		return;
	}
	timeout->tv_sec = 0;
	timeout->tv_usec = (unsigned int)(((double)ALPHA_M_PACKET_TIMEOUT)/2.0 + (((double)ALPHA_M_PACKET_TIMEOUT)/2.0)*(rand() / RAND_MAX));
} //]]

//! Takes a given amount of packets from the queue
/*!
 * The amount of packets taken from the queue is currently given
 * by the variable ALPHA_M_MINIMUM_PACKETS.
 *
 * If there are not enough packets in the supplied queue, nothing will be done.
 * Also nothing is performed if the association is not ready.
 *
 * @param[in,out] ass the association to which the packets shall be added
 * @param[in,out] queue the queue from which packets shall be taken
 * @see alpha_m_ass_flush_queue()
 */
unsigned int alpha_m_ass_pop_packet(alpha_m_ass_t* ass, list_t* queue) {
//[[
	assert(ass != NULL);
	assert(queue != NULL);
	//if we are sending do nothing
	if(list_size(ass->packet_queue) != 0) {
		return 0;
	}
	const unsigned int ls = list_size(queue);
	if(ls < ALPHA_M_MINIMUM_PACKETS) {
		return 0;
	}
	unsigned int packets_to_take = exp_2(log_2(ls));
	unsigned int i;
	size_t packetlen = 0;
	unsigned char* packet = NULL;
	for(i = 0; i < packets_to_take; ++i) {
		 packet = list_pop_front(queue, &packetlen);
		 list_push_front(ass->packet_queue, packet, packetlen);
		 xfree(packet);
	}
	ass->packets_to_send = packets_to_take;
	return packets_to_take;
} //]]

//! Flushes all packets from the given queue
/*!
 * Takes out all packets from the given queue and sends them immidiatly.
 * This only works if the queue is empty! (i.e. this association is not working currently)
 *
 * @param[in] ass the alpha m association, which should be flushed
 * @param[in,out] queue the queue from which packets should be taken
 * @see alpha_m_ass_pop_packet
 */
void alpha_m_ass_flush_queue(alpha_m_ass_t* ass, list_t* queue) {
//[[
	assert(ass != NULL);
	assert(queue != NULL);
	const unsigned int lst_size = list_size(queue);
	if(lst_size == 0) {
		return;
	}
	//check if we have 2^x packets, if we have less than 2^x set this value to 2^x
	//this line looks weird. Get the highest value 2^x such that 2^x <= size of packet queue
	unsigned int packets_to_send = ( lst_size == exp_2(log_2(lst_size)) ) ? lst_size : exp_2(log_2(lst_size) + 1);
	if(packets_to_send == 1) {
		packets_to_send = 2;
	}
	//flush all packets
	unsigned char* packet;
	size_t packetlen;
	while(list_size(queue) > 0) {
		packet = list_pop_front(queue, &packetlen);
		association_add_packet((association_t*)ass, packet, packetlen);
		free(packet);
	}
	//fill up with random values
	unsigned int i;
	unsigned char rand_val[ALPHA_M_RANDOM_PACKET_SIZE];

	for(i = 0; i < (packets_to_send - lst_size); ++i) {
		RAND_bytes(rand_val, ALPHA_M_RANDOM_PACKET_SIZE);
		association_add_packet((association_t*)ass, rand_val, sizeof(unsigned int));
	}
	ass->packets_to_send = lst_size;
} //]]

//! Calculates the hashtree for sending messages
/*!
 * For Alpha M a hash tree needs to be calculated. In the S1 packet, only the
 * root is transmitted. In each S2 packet the nodes/branches to calculate the
 * root with (using also the payload of the S2 packet) are appended to the S2
 * are appended to the S2 packet. Each Alpha M associations holds a pointer
 * to a hash tree which is currently used for sending.
 *
 * @param[in,out] ass the association for which the tree should be calculated
 *
 * @return AP_ERR_SUCCESS
 */
static ap_err_code alpha_m_ass_calculate_tree(alpha_m_ass_t* ass) {
//[[

	list_iterator_t* it = list_begin(ass->packet_queue);
	size_t packet_size;
	unsigned char* packet = NULL;

	size_t max_data_len = 0;
	unsigned int i;
	const unsigned int ls = list_size(ass->packet_queue);
	const unsigned int packets = (ls == exp_2(log_2(ls))) ? ls : exp_2(log_2(ls));
	//get the maximum packet length
	//! \TODO improve

	for(i = 0; i < packets; ++i) {
		list_iterator_get(it, &packet_size);
		if(packet_size > max_data_len) {
			max_data_len = packet_size;
		}
		list_iterator_next(it);
	}
	list_iterator_free(it);

	htree_free(ass->tree);
	ass->tree = htree_init(packets, max_data_len, HASHSIZE, 0, NULL, 0);

	it = list_begin(ass->packet_queue);
	for(i = 0; i < packets; ++i) {
		packet = list_iterator_get(it, &packet_size);
		htree_add_data(ass->tree, (char*)packet, packet_size);
		list_iterator_next(it);
	}
	list_iterator_free(it);
	htree_calc_nodes(ass->tree, alpha_m_ass_internal_leaf_gen, alpha_m_ass_internal_node_gen, NULL);

	return AP_ERR_SUCCESS;

} //]]

//! Sets all important data items of a given association
/*!
 * Initializes a alpha m association.
 *
 * @param[in,out] ass the association to be initialized
 * @param[in] association_id the id of the association (see host.c)
 * @param[in] client_id the id of the client (see host.c)
 * @param[in] dir the direction of this association (only the default association is bidirectional!)
 * @return AP_ERR_SUCCESS
 */
ap_err_code alpha_m_ass_init(alpha_m_ass_t* ass, const config_t *conf, const uint32_t association_id, const uint32_t client_id, const association_direction_t dir, const cache_tree_security_mode_t sec_mode) {
//[[
	assert(ass != NULL);
	association_init((association_t*)ass, conf, association_id, client_id, dir);
	ass->mode = ALPHA_M;
	ass->tree = NULL;
	ass->node_buffer = NULL;
	ass->sec_mode = sec_mode;
	return AP_ERR_SUCCESS;
} //]]

//! Frees all data items in a given association
/*!
 * Frees the data items and the association itself. This function should only be used by the
 * association.c implementation. Use association_free() for freeing a association.
 *
 * @param[in] the association to be freed or NULL
 *
 * @return AP_ERR_SUCCESS
 */
ap_err_code alpha_m_ass_free(alpha_m_ass_t* ass) {
//[[
	if(ass == NULL){
		return AP_ERR_SUCCESS;
	}
	htree_free(ass->tree);
	/* the node buffer may be NULL if the association was created and freed before the node buffer is filled */
	if(ass->node_buffer != NULL){
		xfree(ass->node_buffer);
	}
	xfree(ass);
	return AP_ERR_SUCCESS;
} //]]

//! Transmits a S1 packet using a given alpha m association
/*!
 * This function takes out \f$ 2^x \f$ elements out of the packet queue
 * and calculates a hashtree. The root of the hash tree is transmitted in
 * the S1 packet.
 *
 * @param[in] conf the configuration to be used to send the packet
 * @param[in,out] ass the assocation over which the packet should be transmitted
 *
 * @return Number of bytes send or 0 if no packet has been transmitted
 */
socklen_t alpha_m_ass_send_s1(const config_t* conf, alpha_m_ass_t* ass) {
//[[
	assert(conf != NULL);
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	assert(list_size(ass->packet_queue) > 0);


	if(list_size(ass->packet_queue) < 2) {
		return 0;
	}

	assert(list_size(ass->packet_queue) == exp_2(log_2(list_size(ass->packet_queue))));

	//if not ready, then the previous S1 packet got lost! we do not need to recalculate the tree then
	if(association_get_state((association_t*)ass, ASS_TRANS_MODE_SENDING) == ASS_STATE_READY) {
		//this sets ass->packets_to_send
		alpha_m_ass_calculate_tree(ass);
	}

	//check if we are sending less packets than fit in a tree
	unsigned int sigs_count = ass->packets_to_send;
	if(ass->packets_to_send == 1) {
		sigs_count = 2;
	} else {
		if(ass->packets_to_send != exp_2(log_2(ass->packets_to_send))) {
			sigs_count = exp_2(log_2(ass->packets_to_send) + 1);
		}
	}

	//create packet and send the root (and packets which are needed)


	const unsigned int depth = log_2(sigs_count);
	const size_t alpha_packet_size = sizeof(alpha_packet_s1_m_t) + depth * HASHSIZE;
	alpha_packet_s1_m_t *packet = xmalloc(alpha_packet_size);
	memset(packet, 0, alpha_packet_size);
	packet->association_id = association_get_id((association_t*)ass);
	packet->type = PACKET_S1;
	memcpy(packet->anchor, association_get_sign_element((association_t*)ass), HASHSIZE);

	packet->signatures_count = ass->packets_to_send;
	packet->sec_mode = ass->sec_mode;

	int root_size;
	//append the root to the packet
	memcpy((unsigned char*)packet + sizeof(alpha_packet_s1_m_t), htree_get_root(ass->tree, &root_size), HASHSIZE);

	//append the needed nodes
	unsigned char branch_nodes[depth * HASHSIZE];
	int branch_len;
	//! \TODO OPTIMIZE
	htree_get_branch(ass->tree, 0, branch_nodes, &branch_len);
	memcpy((unsigned char*)packet + sizeof(alpha_packet_s1_m_t) + HASHSIZE, branch_nodes + HASHSIZE, (depth-1) * HASHSIZE);

	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S1  -- anchor: %.*s...\n",
		association_get_client_id((association_t*)ass),
		association_get_id((association_t*)ass),
		DIGEST_PRINTLEN,
		digeststr(packet->anchor, NULL));

	socklen_t ret_val = ap_protocol_send_udp(conf, association_get_client_id((association_t*)ass), packet, alpha_packet_size);
	xfree(packet);
	return ret_val;

} //]]

//! Handles an incoming Alpha M S1 packet
/*!
 * Stores the root which is included in the S1 packet. Additionally the packets to be sent are also
 * transmitted in the S1 packet and stored in the association.
 *
 * @param[in, out] ass the association on which the packet came in
 * @param[in] packet pointer to be beginning of the packet
 * @param[in] payload pointer which points after the header
 * @param[in] payload_size size of the packet - the alpha s1 header
 *
 * @return AP_ERR_SUCCESS
 */
ap_err_code alpha_m_ass_handle_s1(alpha_m_ass_t* ass, const alpha_packet_s1_t* packet, const unsigned char* payload, const size_t payload_size) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_size > 0);
	const alpha_packet_s1_m_t* packet_m = (const alpha_packet_s1_m_t*)packet;
	ass->packets_to_receive = packet_m->signatures_count;
	ass->packets_received = 0;
	ass->sec_mode = packet_m->sec_mode;
	//store the root
	memcpy(ass->root, (const unsigned char*)packet + sizeof(alpha_packet_s1_m_t), HASHSIZE);
	const unsigned int depth = log_2(ass->packets_to_receive);
	if(ass->packets_to_receive > 2){
		ass->node_buffer = cache_tree_reallocate_node_buffer(ass->node_buffer, ass->packets_to_receive, HASHSIZE);
		if(ass->node_buffer == NULL) {
			return AP_ERR_NOMEM;
		}
		//store the log_2(n)-1 nodes
		const unsigned char* nodes = (const unsigned char*)packet + sizeof(alpha_packet_s1_m_t) + HASHSIZE;
		unsigned int i;
		for(i = 0; i < (depth - 1); ++i) {
			cache_tree_store_node(ass->node_buffer, nodes + (i * HASHSIZE), 1, (i+1), ass->packets_to_receive, HASHSIZE);
		}
	}
	return AP_ERR_SUCCESS;
} //]]

//! Sends all S2 packets
/*!
 * Sends all Alpha M S2 packets containing the branches and the payload. Each packet looks like this:
 *
 * [Alpha S2 Headern (fixed size)][Branches (variable)][Payload (variable)]
 *
 * The number of packets sent depends on the number of packets in the queue.
 * The number of packets sent is always \f$ 2^x \f$ where \f$ x \f$ is a posivite integer.
 *
 * @param[in] conf the configuration to be used to send the packet
 * @param[in,out] ass the association to be used to send the packet
 * @return the number of bytes sent
 */
socklen_t alpha_m_ass_send_s2(const config_t* conf, alpha_m_ass_t* ass) {
//[[
	assert(conf != NULL);
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	assert(list_size(ass->packet_queue) > 0);
	// send all S2 packets

	// unsigned char* send_branches = NULL;
	unsigned int i;

	// send all packets
	alpha_packet_s2_m_t* packet = NULL;
	unsigned char* payload;
	size_t payload_size;
	socklen_t total_bytes = 0;

	const unsigned int signatures_per_packet = cache_tree_get_number_of_sent_signatures(ass->sec_mode);
	unsigned int packets = ass->packets_to_send;
	/* Alpha M only allows to send bursts of 2^x packets. We have to create dummy packets*/
	if(ass->packets_to_send == 1) {
		/* special case for 2^0 */
		packets = 2;
	} else {
		if(ass->packets_to_send != exp_2(log_2(ass->packets_to_send))) {
			packets = exp_2(log_2(ass->packets_to_send) + 1);
		}
	}

	/* TODO: This could be handled in paralell so that the protocol flow is not interrupted for long*/
	for(i = 0; i < ass->packets_to_send; ++i) {
		payload = list_pop_front(ass->packet_queue, &payload_size);

		const size_t header_size = sizeof(alpha_packet_s2_m_t) + HASHSIZE * signatures_per_packet;
		const size_t alpha_packet_size = header_size + payload_size;
		packet = xmalloc(alpha_packet_size);


		unsigned char* branch_ptr = (unsigned char*)packet + sizeof(alpha_packet_s2_m_t);

		cache_tree_get_nodes(ass->tree, packets, i, conf->alpha_m_sec_mode, HASHSIZE, &branch_ptr);

		packet->type = PACKET_S2;
		packet->association_id = association_get_id((association_t*)ass);
		memcpy(packet->anchor, association_get_sign_element((association_t*)ass), HASHSIZE);

		unsigned char* payload_ptr = (unsigned char*)packet + header_size;
		packet->data_index = i;

		//payload
		memcpy(payload_ptr, payload, payload_size);

		//send packet
		char anchor_buf[2*HASHSIZE+1];
		AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S2  -- anchor: %.*s..., branches: %.*s...\n",
			association_get_client_id((association_t*)ass),
			packet->association_id,
			DIGEST_PRINTLEN, digeststr(packet->anchor,anchor_buf),
			DIGEST_PRINTLEN, digeststr(branch_ptr, NULL)
		);

		total_bytes += ap_protocol_send_udp(conf, association_get_client_id((association_t*)ass), packet, alpha_packet_size);

		xfree(payload);
		xfree(packet);
	}

	// empty the queue
	for(i = 0; i < (packets - ass->packets_to_send); ++i) {
		payload = list_pop_front(ass->packet_queue, &payload_size);
		free(payload);
	}

	return total_bytes;
} //]]

//! Handles an incoming S2 packet
/*!
 * Checks whether the incoming S2 packet is valid by verifying the
 * given branches (which are contained in the packet).
 *
 * The payload pointer and the payload_len refer to the packet payload
 * where the header is a default s2 header. But in the 'payload' there
 * are also the branches stored. Therefore this function returns
 * pointers which do point to the 'real' payload.
 *
 * @param[in] ass the association on which the packet is incoming
 * @param[in] packet the incoming packet (points to the default alpha s2 header)
 * @param[in] payload points behind the default alpha s2 header
 * @param[in] payload_len length of the packet minus the default alpha s2 header
 * @param[out] valid whether the packet is valid regarding the branches (hash chains are not verified here!)
 * @param[out] real_payload pointer which points behind the branches of the alpha m s2 packet
 * @param[out] real_payload_size the size of the entire packet beginning from the point after the branches
 *
 * @return AP_ERR_SUCCESS
 */
ap_err_code alpha_m_ass_handle_s2(alpha_m_ass_t* const ass, const alpha_packet_s2_t* const packet,
	unsigned char* const payload, const size_t payload_len, bool* const valid, unsigned char** real_payload,
	size_t* const real_payload_size) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_len > 0);
	assert(valid != NULL);

	*valid = false;

	const alpha_packet_s2_m_t* packet_m = (const alpha_packet_s2_m_t*)packet;

	const uint16_t data_index = packet_m->data_index;
	size_t packets = ass->packets_to_receive;
	if(ass->packets_to_receive == 1) {
		packets = 2;
	} else {
		if(ass->packets_to_receive != exp_2(log_2(ass->packets_to_receive))) {
			packets = exp_2(log_2(ass->packets_to_receive) + 1);
		}
	}

	const unsigned int depth = log_2(packets);
	// the expected payload pointer points to data items in the alpha packet s2 m
	unsigned char* branch_nodes = payload + (sizeof(alpha_packet_s2_m_t) - sizeof(alpha_packet_s2_t));
	const unsigned int branches_count = cache_tree_get_number_of_sent_signatures(ass->sec_mode);
	size_t real_payload_len = payload_len - (branches_count * HASHSIZE) - (sizeof(alpha_packet_s2_m_t) - sizeof(alpha_packet_s2_t));
	unsigned char* real_payload_ptr = branch_nodes + (branches_count * HASHSIZE);

#if AP_MSG_LVL_VERBOSE == AP_MSG_LVL
	char anchor_buf[2*HASHSIZE+1];
	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got S2      -- anchor: %.*s..., data index: %u, nodes received: %u, first node: %.*s...\n",
		association_get_client_id((association_t*)ass), association_get_id((association_t*)ass),
		DIGEST_PRINTLEN, digeststr(packet->anchor, anchor_buf),
		data_index,
		branches_count,
		DIGEST_PRINTLEN, digeststr(branch_nodes, NULL)
	);
#endif

	unsigned char buf[log_2(packets) * HASHSIZE];
	memset(buf, 0, log_2(packets) * HASHSIZE);
	unsigned char* buf_ptr = buf;

	cache_tree_build_verification_buffer((const unsigned char *)ass->node_buffer,
		branch_nodes, &(buf_ptr), data_index, packets, ass->sec_mode, HASHSIZE);

	unsigned char calc_node_buffer[log_2(packets) * HASHSIZE];

	if(htree_verify_branch(ass->root, HASHSIZE, buf_ptr, depth * HASHSIZE, real_payload_ptr, real_payload_len,
		data_index, NULL, 0, alpha_m_ass_internal_leaf_gen, alpha_m_ass_internal_node_gen, NULL,
		calc_node_buffer + HASHSIZE, calc_node_buffer) == 0) {
		*valid = true;
		ass->packets_received++;

		cache_tree_update_buffer(ass->node_buffer, data_index, packets, branch_nodes, calc_node_buffer, ass->sec_mode, HASHSIZE);

		//check if all packets have came in
		if(ass->packets_received >= ass->packets_to_receive) {
			association_set_state((association_t*)ass, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);
			association_dec_ack_rounds((association_t*)ass);
		}

	} else {
		*valid = false;

		//update buffer if possible
		if(ass->sec_mode == CACHE_TREE_TWO_SIGNATURES_PER_PACKET) {

			if(cache_tree_verify_new_cache_entry(ass->node_buffer, branch_nodes,
				branch_nodes + HASHSIZE, data_index, packets, alpha_m_ass_internal_node_gen,
				NULL, HASHSIZE)) {

				cache_tree_update_buffer(ass->node_buffer, data_index, packets, branch_nodes, calc_node_buffer, ass->sec_mode, HASHSIZE);

			}
		}
		return AP_ERR_SUCCESS;
	}

	*real_payload = real_payload_ptr;
	*real_payload_size = real_payload_len;

	return AP_ERR_SUCCESS;
} //]]

//! Leaf generator for alpha m
/*!
 * Uses the create_digest function to generate leafs for the hash tree used in ALPHA M.
 *
 * @param[in] data data item to be hashed
 * @param[in] data_length size of the data item
 * @param[in] secret secrets which can be optionally used (see Alpha Paper by Tobias Heer)
 * @param[in] secret_length size of the secret
 * @param[in] dst_buffer buffer for the generated leaf node (i.e. the result)
 * @param[in] gen_args arguments for the generator (not used)
 * @return always 0
 * @see htree_leaf_generator()
 */
int alpha_m_ass_internal_leaf_gen(const unsigned char* data, int data_length, unsigned char* secret, int secret_length,
	unsigned char* dst_buffer, htree_gen_args_t* args) {
//[[
	int err = 0;
	unsigned char buffer[data_length + secret_length];
	int hash_data_length = 0;
	const unsigned char *hash_data = NULL;

	if(secret && secret_length > 0) {
		memcpy(&buffer[0], data, data_length);
		memcpy(&buffer[data_length], secret, secret_length);

		hash_data = buffer;
		hash_data_length = data_length + secret_length;

	} else {
		hash_data = data;
		hash_data_length = data_length;
	}

	if(!create_digest((const unsigned char *)hash_data, hash_data_length, dst_buffer))
		HT_ERROR("failed to calculate hash\n");

	out_err:
		return err;
}//]]

//! Node generator for alpha m
/*!
 * Uses the create_digest function to generate nodes for the hash tree used in ALPHA M.
 *
 * @param[in] left_node the left child node of the node which should be generated
 * @param[in] right_node the right child node of the node which should be generated
 * @param[in] node_length the length of the nodes in the tree (i.e. the function of the hash function)
 * @param[out] dst_buffer pre-allocated space to store the node (space is node_length size)
 * @param[in] args not used but specified in hashtree.h
 */
int alpha_m_ass_internal_node_gen(const unsigned char* left_node, const unsigned char* right_node, int node_length,
	unsigned char* dst_buffer, htree_gen_args_t* args) {
//[[
	int err = 0;

	/* the calling function has to ensure that left and right node are in
	 * subsequent memory blocks */
	if(!create_digest(left_node, 2 * node_length, dst_buffer)){
		print_error("failed to calculate hash\n");
	}
	return err;

}//]]
