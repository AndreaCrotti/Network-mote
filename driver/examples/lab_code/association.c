// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <assert.h>

#include "alpha_n.h"
#include "alpha_c.h"
#include "alpha_m.h"
#include "alpha_z.h"
#include "tools.h"
#include "digest.h"
#include "timemanager.h"
#include "xmalloc.h"
#include "control_association.h"

//********************************
// SIMPLE ACCESSOR-METHODS
//********************************

//! Gets the current element from the sign hash chain
/*!
 * @param[in] ass the association, whose current sign element should be returned
 * @return the current sign hash chain element
 */
inline unsigned char* association_get_sign_element(const association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->sign_hash_chain != NULL);
	return hchain_current(ass->sign_hash_chain);
} //]]

//! Gets the current element from the ack hash chain
/*!
 * @param[in] ass the association, whose current ack element should be returned
 * @return the current ack hash chain element
 */
inline unsigned char* association_get_ack_element(const association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->ack_hash_chain != NULL);
	return hchain_current(ass->ack_hash_chain);
} //]]

//! Wrapper function to the the id of a given association
/*!
 * @param[in] ass the association, whose id should be returned
 * @return the id of the association
 */
inline uint32_t association_get_id(const association_t* ass) {
//[[
	assert(ass != NULL);
	return ass->id;
} //]]

//! Gets the state of a specific association mode
/*!
 * @param[in] association The association which should be checked
 * @param[in] mode For which mode the state should be checked
 * @return The state of the association
 */
inline association_state_t association_get_state(const association_t* ass, const association_transmission_mode_t mode) {
//[[
	assert(mode < ASS_TRANS_MODE_COUNT);
	assert(ass != NULL);
	return ass->state[mode];
} //]]

//! Sets the state of a specific association mode
/*!
 * @param[in,out] association The association whose mode needs to be set
 * @param[in] mode the mode which is concerned
 * @param[in] state the new state
 * @return AP_ERR_SUCCESS
 */
inline ap_err_code association_set_state(association_t* ass, const association_transmission_mode_t mode, const association_state_t state) {
//[[
	assert(mode < ASS_TRANS_MODE_COUNT);
	assert(ass != NULL);
	assert(state < ASS_STATE_COUNT);
	ass->state[mode] = state;
	return AP_ERR_SUCCESS;
} //]]

//! Gets the running mode of this association
/*!
 * The running mode is either alpha M, N or C. Do not confuse this mode with
 * the transmission mode (association_transmission_mode).
 *
 * \see alpha_mode_t
 *
 * @param[in] ass the association which needs to be checked
 * @return the alpha mode
 */
alpha_mode_t association_get_mode(const association_t* ass) {
//[[
	assert(ass != NULL);
	return ass->mode;
} //]]
//! Decrements the ACK hash chain
/*!
 * Pops an element from the ack hash chain from the given association.
 * @param[in,out] ass the association, whose hash chain should be modified
 */
inline void association_dec_ack_rounds(association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->ack_hash_chain != NULL);
	hchain_pop(ass->ack_hash_chain);
} //]]

//! Decrements the SIGN hash chain
/*!
 * Pops an element from the ack hash chain from the given association.
 * @param[in,out] ass the association, whose hash chain should be modified
 */
inline void association_dec_sign_rounds(association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->sign_hash_chain != NULL);
	hchain_pop(ass->sign_hash_chain);
} //]]

//! get the associated alpha client data
/*!
 * Returns the alpha client data, which uses this association.
 *
 * @param[in] ass the association whose alpha client data should be returned
 * @return a pointer to the client
 */
inline uint32_t association_get_client_id(const association_t* ass) {
//[[
	assert(ass != NULL);
	return ass->client_id;
} //]]

//! Gets the timestamp for the given association
/*!
 * Timestamps are used care about timeouts.
 * @param[in] ass the association whose timeout should be returned
 * @see association_handle_timeouts()
 */
inline time_t association_get_timestamp(const association_t* ass) {
//[[
	assert(ass != NULL);
	return ass->timestamp;
} //]]

//! Sets the timestampe for the given association
/*!
 * Timestamps are used care about timeouts.
 * @param[in,out] ass the association whose timestamp should be set
 * @param[in] time the new timestamp
 * @see association_handle_timeouts()
 */
inline void association_set_timestamp(association_t* ass, const time_t return_time) {
//[[
	assert(ass != NULL);
	ass->timestamp = return_time;
} //]]

//! Inserts an sign element into the ring buffer
/*!
	Adds a sign element into the associations' ring buffer. Sign elements are also known as
	sign anchors. They are used as a key for the HMAC and are used to keep the connection
	alive. Elements that are added are elements from the opposing hash chain.

	@param[in,out] ass the association to which the sign element shall be added

	@remark due to limited size of the ring buffer old elements might get overwritten (aging of elements)

 */
inline ap_err_code association_add_sign_element(association_t* ass, const unsigned char* sign_element) {
//[[
	assert(ass != NULL);
	assert(sign_element != NULL);
	return ring_buffer_insert(ass->sign_anchors, sign_element);
} //]]

//! Inserts an ack element into the ring buffer
/*!
	Adds an ack element into the associations' ring buffer. Ack elements are also known as
	ack anchors. They are used to keep the connection
	alive. Elements that are added are elements from the opposing hash chain.

	@param[in,out] ass the association to which the ack element shall be added

	@remark due to limited size of the ring buffer old elements might get overwritten (aging of elements)

 */
inline ap_err_code association_add_ack_element(association_t* ass, const unsigned char* ack_element) {
//[[
	assert(ass != NULL);
	assert(ack_element != NULL);
	return ring_buffer_insert(ass->ack_anchors, ack_element);
} //]]
//! Finds an anchor in a given ring buffer
/*!
 *  This function hashes the anchor using create_digest() and then searches for this
 *  element in the given ring buffer using ring_buffer_find_and_move()
 *
 *  @param[in,out] buf the ring buffer to be searched
 *  @param[in] anchor the anchor to be hashed and searched for
 *  @return true if found
 */
static inline bool association_find_anchor(ring_buffer_t* buf, const unsigned char* anchor) {
//[[
	assert(buf != NULL);
	assert(anchor != NULL);
	unsigned char calc_anchor[HASHSIZE];
	create_digest(anchor, HASHSIZE, calc_anchor);
	if(ring_buffer_find_and_move(buf, calc_anchor)) {
		return true;
	} else {
		//try double hashing
		unsigned char double_hashed[HASHSIZE];
		create_digest(calc_anchor, HASHSIZE, double_hashed);
		return ring_buffer_find_and_move(buf, double_hashed);
	}
} //]]

//! Compares anchors
/*!
 * Given a element/anchor a and a next anchor b this function checks if h(a) = b.
 *
 * @param[in] anchor this is a (i.e. the anchor on which the hash function is applied)
 * @param[in] next_anchor this is b (i.e. the anchor which is compared to hashfunction(anchor))
 * @return true if h(a) = b
 */
/* TODO: Check if this is still needed */
inline bool association_verify_anchor(const unsigned char* anchor, const unsigned char* next_anchor) {
//[[
	unsigned char calc_anchor[HASHSIZE];
	create_digest(anchor, HASHSIZE, calc_anchor);
	return (memcmp(calc_anchor, next_anchor, HASHSIZE) == 0);
} //]]

//! Initializes a association (ONLY FOR INTERNAL USE (see below))
/*!
 * This function should be called from different alpha modi which
 * bring their own association structure (like alpha_n_ass_t).
 * This function initializes hash chain and so on.
 *
 * @param[in, out] ass a already allocated association
 * @param[in] client_id the id of the client who uses this association
 * @param[in] association_id the id of this association
 *
 * @note use alpha_n_ass_init(), alpha_m_ass_init(), alpha_c_ass_init() instead of this function!
 *
 * @return AP_ERR_SUCCESS if everything went fine, otherwise AP_ERR_NOMEM if not enough mem is given
 */
ap_err_code association_init(association_t* ass, const config_t *conf, const uint32_t association_id, const unsigned int client_id, const association_direction_t dir) {
//[[
	assert(ass != NULL);
	ass->sign_hash_chain = NULL;
	ass->ack_hash_chain = NULL;
	ass->ack_anchors = NULL;
	ass->sign_anchors = NULL;
	ass->packet_queue = NULL;

	ass->id = association_id;
	ass->direction = dir;

	ap_err_code result = association_init_new_chains(ass, conf, dir);

	if(result != AP_ERR_SUCCESS)
		return result;
	
	if(dir != ASS_DIRECTION_INCOMING) {
		ass->packet_queue = list_new(NULL);
		if(!ass->packet_queue) {
			return AP_ERR_NOMEM;
		}
	}

	ass->ack_anchors = ring_buffer_new(ASS_ACK_ANCHORS_BUFSIZE, HASHSIZE);
	ass->sign_anchors = ring_buffer_new(ASS_SIGN_ANCHORS_BUFSIZE, HASHSIZE);

	int i = 0;
	for(i = 0; i < ASS_TRANS_MODE_COUNT; ++i) {
		ass->state[i] = ASS_STATE_NEW;
	}

	ass->client_id = client_id;

	memset(ass->ret_anchor, 0, HASHSIZE);
	return AP_ERR_SUCCESS;
} //]]

//! Frees a given association depending on the mode the association has
/*!
 * @param[in,out] ass The association to be freed
 * @return AP_ERR_STATE if the mode of the association is not supported by the function,
 * 	otherwise the value which is returned by the free function of the mode implementation
 */
ap_err_code association_free(association_t* ass) {
//[[
	if(ass == NULL) {
		return AP_ERR_SUCCESS;
	}
	hchain_free(ass->sign_hash_chain);
	hchain_free(ass->ack_hash_chain);
	list_free(ass->packet_queue);
	ring_buffer_free(ass->sign_anchors);
	ring_buffer_free(ass->ack_anchors);
	switch(association_get_mode(ass)) {
		case ALPHA_N:
			return alpha_n_ass_free((alpha_n_ass_t*)ass);
		case ALPHA_M:
			return alpha_m_ass_free((alpha_m_ass_t*)ass);
		case ALPHA_C:
			return alpha_c_ass_free((alpha_c_ass_t*)ass);
		default:
			return AP_ERR_STATE;
	}
} //]]

//! Gets the number of packets in the queue
/*!
 * @param[in] association The association which should be checked
 * @return the number of packets in the queue
 */
inline size_t association_get_queue_size(const association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	return list_size(ass->packet_queue);
} //]]

//! Moves a packet queue from a association to another one
/*!
 *  Performs a packet queue migration, while all other association data stays the same.
 *
 *  @param[in,out] destination association where the packets from source should be copied to
 *  @param[in,out] source association where the packets should be taken from and copied to destiniation
 *  @return AP_ERR_SUCCESS
 */
ap_err_code association_move_packet_queue(association_t* destination, association_t* source) {
//[[
	assert(destination != NULL);
	assert(source != NULL);
	size_t packet_len = 0;
	unsigned char* packet = NULL;
	while(association_get_queue_size(source) > 0) {
		packet = list_pop_front(source->packet_queue, &packet_len);
		list_push_back(destination->packet_queue, packet, packet_len);
		xfree(packet);
	}
	return AP_ERR_SUCCESS;
} //]]

//! Adds a packet to the association queue
/*!
 * @param[in,out] ass the association, to which the packet should be added
 * @param[in] packet the packet to be added
 * @param[in] len the size of the packet
 *
 * @return AP_ERR_SUCCESS
 */
ap_err_code association_add_packet(association_t* ass, unsigned char* packet, const size_t len) {
//[[
	// check given parameters
	assert(ass != NULL);
	assert(packet != NULL);
	assert(len > 0);
	// check if association has been inited
	assert(ass->packet_queue != NULL);
	list_push_back(ass->packet_queue, packet, len);
	return AP_ERR_SUCCESS;
} //]]

//! Sends a S1 packet regarding the mode of the association
/*!
 * Depending on the mode, different S1 packets are sent.
 *
 * @param[in] ass The association, over which the packet should be transmitted
 * @param[in] conf The configuration used to sent the packet
 *
 * @return number of sent bytes
 *
 * \msc
 * A,B;
 * A->B [label="Sign Chain Element, MAC(message)"];
 * \endmsc
 */
socklen_t association_send_s1(association_t* ass, const config_t* conf) {
//[[
	assert(ass != NULL);
	assert(conf != NULL);
	socklen_t retval = 0;
	association_set_timestamp(ass, time(NULL));

	switch(association_get_mode(ass)) {
		case ALPHA_N:
			retval = alpha_n_ass_send_s1(conf, (alpha_n_ass_t*)ass);
			break;
		case ALPHA_C:
			retval = alpha_c_ass_send_s1(conf, (alpha_c_ass_t*)ass);
			break;
		case ALPHA_M:
			retval = alpha_m_ass_send_s1(conf, (alpha_m_ass_t*)ass);
			break;
		case ALPHA_Z:
			/* Here we send an S2 packet because ALPHA Z is just passing packets w/o authentication */
			retval = alpha_z_ass_send_s2(conf, (alpha_z_ass_t*)ass);
		default:
			return 0;

	}
	if(retval > 0) {
		association_set_state(ass, ASS_TRANS_MODE_SENDING, ASS_STATE_SENT_S1_WAIT_A1);
	}
	return retval;
} //]]

//! Sends a A1 packet
/*!
 * Sends a A1 packet, which includes a ack anchor and the received sign anchor.
 *
 * @param[in,out] ass the association, which is used to transmit the packet (hash chains are modified therefore not const)
 * @param[in] conf the configuration which is used to sent the packet
 * @return number of sent bytes
 *
 * \msc
 * A,B;
 * B<-A [label="Ack Chain Element, Received Sign Element"];
 * \endmsc
 *
 */
socklen_t association_send_a1(association_t* ass, const config_t* conf) {
//[[
	assert(ass != NULL);
	assert(conf != NULL);
	assert(ass->ack_hash_chain != NULL);

	alpha_packet_a1_t packet;
	packet.type = PACKET_A1;
	packet.association_id = association_get_id(ass);

	const unsigned int client_id = association_get_client_id(ass);

	// This should never happen
	if(association_remaining_anchors_ack(ass) <= MIN_ANCHORS) {
		AP_MSG_F(AP_MSG_LVL_ERROR, AP_MSG_CTX_ALL, "<%d, %3d> We ran out of ACK anchors, wonder how that happened!\n",
			client_id, packet.association_id);
		return AP_ERR_STATE;
	}

	// Send new ACK-anchor and return the sent SIGN-anchor

	// get current ack anchor
	memcpy(packet.anchor, hchain_current(ass->ack_hash_chain), HASHSIZE);
	// get last sign anchor
	memcpy(packet.return_anchor, ass->ret_anchor, HASHSIZE);


	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending A1  -- anchor: %.*s...\n",
		client_id, packet.association_id, DIGEST_PRINTLEN, digeststr(packet.anchor, NULL));

	association_set_state(ass, ASS_TRANS_MODE_RECEIVING, ASS_STATE_SENT_A1_WAIT_S2);
	return ap_protocol_send_udp(conf, client_id, &packet, sizeof(alpha_packet_a1_t));
} //]]

//! Sends a S2 packet regarding the mode of the association
/*!
 * Depending on the mode, S2 packets are sent.
 *
 * @param[in] ass The association, over which the packet should be transmitted
 * @param[in] conf The configuration used to sent the packet
 *
 * @return number of sent bytes
 *
 * \msc
 * A,B;
 * A->B [label="new sign chain element, message"];
 * \endmsc
 */
socklen_t association_send_s2(association_t* ass, const config_t* conf) {
//[[
	assert(ass != NULL);
	assert(conf != NULL);

	socklen_t retval=0;

	if(association_remaining_anchors_sign(ass) <= MIN_ANCHORS) {
		unsigned char* packet = NULL;
		size_t packet_size;
		unsigned char client_id = association_get_client_id(ass);
		if(association_get_id(ass) == 0) {
			//the default association has to be updated!
			if(host_get_new_default_association(client_id) == NULL) {
				association_t* new_def_ass = (association_t *)host_create_new_default_association(client_id, conf);
				//take all packets to the new queue
				association_move_packet_queue(new_def_ass, ass);
				ca_req_ctrl_ass(client_id, new_def_ass);
				return 0;
			}
		} else {
			//this association has to die!
			//take all packets back to the host
			while(list_size(ass->packet_queue) > 0){
				packet = list_pop_front(ass->packet_queue, &packet_size);
				host_enqueue_packet(client_id, packet, packet_size);
				xfree(packet);
			}

			unsigned int ass_id = association_get_id(ass);
			ca_kill_ass(client_id, conf, &ass_id, 1);
			return AP_ERR_SUCCESS;
		}


	}

	switch(association_get_mode(ass)) {
		case ALPHA_M:
			retval = alpha_m_ass_send_s2(conf, (alpha_m_ass_t*)ass);
			break;
		case ALPHA_C:
			retval = alpha_c_ass_send_s2(conf, (alpha_c_ass_t*)ass);
			break;
		case ALPHA_N:
			retval = alpha_n_ass_send_s2(conf, (alpha_n_ass_t*)ass);
			break;
		case ALPHA_Z:
			// this never happends; packets are already transmitted when sending S1
			assert(1 == 0);
			break;
	}

	association_dec_sign_rounds(ass);
	association_set_state(ass, ASS_TRANS_MODE_SENDING, ASS_STATE_READY);

	if(association_get_id(ass) != 0) {
		host_insert_ready_association(association_get_client_id(ass), (association_t*)ass);
	}

	return retval;
} //]]

//! Handles an incoming S1 packet
/*!
 * Depending on the running mode, this function handles incoming S1 packets.
 *
 * @param[in,out] ass the association to be used
 * @param[in] config the configuration to be used
 * @param[in] packet the incoming alpha S1 packet
 * @param[in] payload_size the size of the payload in bytes (does NOT include the size of the alpha_packet_s1)
 * @param[out] valid already allocated space to store whether this packet is valid
 */
ap_err_code association_handle_s1(association_t* ass, const config_t* conf, const alpha_packet_s1_t* packet, const size_t payload_size, bool* valid) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(valid != NULL);

	*valid = false;

// DEBUG PRINTING
#if AP_MSG_LVL == AP_MSG_LVL_VERBOSE
	unsigned char temp_buf[HASHSIZE];
	create_digest(packet->anchor, HASHSIZE, temp_buf);
	char anchor_out[2*HASHSIZE+1], temp_buf_out[2*HASHSIZE+1], sign_anchor_out[2*HASHSIZE+1], hmac_out[2*HASHSIZE+1];
		AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got S1      -- anchor: %.*s..., hash: %.*s..., expected: %.*s..., HMAC: %.*s...\n",
		association_get_client_id(ass),
		association_get_id(ass),
		DIGEST_PRINTLEN, digeststr(packet->anchor, anchor_out),
		DIGEST_PRINTLEN, digeststr(temp_buf, temp_buf_out),
		DIGEST_PRINTLEN, digeststr(ring_buffer_const_read(ass->sign_anchors), sign_anchor_out),
		DIGEST_PRINTLEN, digeststr((const unsigned char*)packet + sizeof(alpha_packet_s1_t), hmac_out)
		);
#endif
// END DEBUG PRINTING

	// check for validity
	if(!association_find_anchor(ass->sign_anchors, packet->anchor)) {
		*valid = false;
		return AP_ERR_SUCCESS;
	} else {
		*valid = true;
	}

	// FROM HERE ON VALID ANCHORS ARE ASSUMED

	// update anchors
	association_add_sign_element(ass, packet->anchor);
	ring_buffer_read(ass->sign_anchors);

	memcpy(ass->ret_anchor, packet->anchor, HASHSIZE);

	ap_err_code retval;

	// do some mode specific operations
	// (e.g. getting signatures)
	switch(ass->mode) {
		case ALPHA_N:
			retval = alpha_n_ass_handle_s1((alpha_n_ass_t*)ass, packet);
			break;
		case ALPHA_C:
			retval = alpha_c_ass_handle_s1((alpha_c_ass_t*)ass, packet, (const unsigned char*)packet + sizeof(alpha_packet_s1_t), payload_size);
			break;
		case ALPHA_M:
			retval = alpha_m_ass_handle_s1((alpha_m_ass_t*)ass, packet, (const unsigned char*)packet + sizeof(alpha_packet_s1_t), payload_size);
			break;
		default:
			return AP_ERR_INVALID_MODE;
	}
	if(retval != AP_ERR_SUCCESS) {
		return retval;
	}

	association_send_a1(ass, conf);
	association_set_state(ass, ASS_TRANS_MODE_RECEIVING, ASS_STATE_SENT_A1_WAIT_S2);

	return AP_ERR_SUCCESS;
} //]]

//! Verifies a S2 packet in ALPHA
/*!
 * Depending on the mode this function chooses a function which verifies the
 * given S2 packet. Depending on the mode, the payload content may differ,
 * (i.e. there might be additional data in the payload which rather belongs
 * to the ALPHA header).
 *
 * This function updates the sign chain element and decrements the ACK rounds.
 *
 * @param[in] ass the association on which this packets comes in
 * @param[in] packet the actual ALPHA S2 packet
 * @param[in] payload the payload of the alpha S2 packet (does NOT include the alpha S2 header) this might include some verfication data (e.g. branches from alpha M)
 * @param[in] payload_len the size of the payload
 * @param[out] valid pre-allocated space to store wheter this packet is valid
 * @param[out] real_payload A pointer to the actual payload
 * @param[out] real_payload_size Size of the real payload
 *
 * @return 0 if the signature is valid
 *
 * @see application.h, host_verify_s2_alpha_m(), host_verify_s2_alpha_normal()
 */
ap_err_code association_handle_s2(association_t* ass, const config_t *conf, const alpha_packet_s2_t* packet, unsigned char* payload, const size_t payload_len,
	bool* const valid, unsigned char** real_payload, size_t* const real_payload_size) {
//[[
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_len > 0);
	assert(valid != NULL);

	*valid = false;
	ap_err_code retval;

	//check hash
	//if(!association_verify_anchor(packet->anchor, ass->sign_anchor)) {
	if(ass->mode != ALPHA_Z && !association_find_anchor(ass->sign_anchors, packet->anchor)) {
		*valid = false;
		return AP_ERR_SUCCESS;
	}

	// note! the new state is set in the associations, because e.g. ALPHA C needs to wait for multiple S2
	// additionally hash rounds are decremented (for the ACK) in the functions
	switch(ass->mode) {
		case ALPHA_N:
			retval = alpha_n_ass_handle_s2((alpha_n_ass_t*)ass, packet, payload, payload_len, valid, real_payload, real_payload_size);
			break;
		case ALPHA_C:
			retval = alpha_c_ass_handle_s2((alpha_c_ass_t*)ass, packet, payload, payload_len, valid, real_payload, real_payload_size);
			break;
		case ALPHA_M:
			retval = alpha_m_ass_handle_s2((alpha_m_ass_t*)ass, packet, payload, payload_len, valid, real_payload, real_payload_size);
			break;
		case ALPHA_Z:
			retval = alpha_z_ass_handle_s2((alpha_z_ass_t*)ass, packet, payload, payload_len, valid, real_payload, real_payload_size);
			*valid = true;
			break;
		default:
			return AP_ERR_INVALID_MODE;
	}
	if(retval != AP_ERR_SUCCESS) {
		return retval;
	}
	if(!(*valid)) {
		return AP_ERR_SUCCESS;
	}

	// we have a valid packet from now on


	// set new anchor in the sign hash chain
	if(!ring_buffer_find(ass->sign_anchors, packet->anchor)) {
		association_add_sign_element(ass, packet->anchor);
		ring_buffer_read(ass->sign_anchors);
	}

	if(association_remaining_anchors_ack(ass) <= MIN_ANCHORS) {
		unsigned int ass_id = association_get_id(ass);
		unsigned int client_id = association_get_client_id(ass);
		if(ass_id == 0) {
			//the default association has to be updated!
			if(host_get_new_default_association(client_id) == NULL) {
				association_t* new_def_ass = (association_t *)host_create_new_default_association(client_id, conf);
				//take all packets to the new queue
				association_move_packet_queue((association_t*)new_def_ass, ass);
				ca_req_ctrl_ass(client_id, new_def_ass);
				return 0;
			}
		}
	}

	return AP_ERR_SUCCESS;
} //]]

//! Handles an incoming A1 packet
/*!
 * If the incoming packet is valid the ACK chain element will be updated.
 *
 * @param[in,out] ass the association on which the A1 packet comes in
 * @param[in] conf the configuration to be used (to sent the S2 packet)
 * @param[in] packet the incoming alpha A1 packet
 * @param[out] valid already allocated space, where the validity of the packet is stored
 *
 * @return ErrorCode
 */
ap_err_code association_handle_a1(association_t* ass, const config_t* conf, const alpha_packet_a1_t* packet, bool* valid) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(valid != NULL);

// DEBUG PRINTING
#ifdef AP_MSG_LVL_VERBOSE
	unsigned char temp_buf[HASHSIZE];
	char anchor_out[2*HASHSIZE+1], temp_buf_out[2*HASHSIZE+1], ack_anchor_out[2*HASHSIZE+1];
	create_digest(packet->anchor, HASHSIZE, temp_buf);
	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got A1      -- anchor: %.*s..., hash: %.*s..., expected: %.*s...\n",
		association_get_client_id(ass),
		association_get_id(ass),
		DIGEST_PRINTLEN, digeststr(packet->anchor, anchor_out),
		DIGEST_PRINTLEN, digeststr(temp_buf, temp_buf_out),
		DIGEST_PRINTLEN, digeststr(ring_buffer_const_read(ass->ack_anchors), ack_anchor_out)
		);
#endif
// END DEBUG PRINTING

	// verify ack anchor && verify received sign anchor
	//if(association_verify_anchor(packet->anchor, ass->ack_anchor) &&
	if(association_find_anchor(ass->ack_anchors, packet->anchor) &&
		memcmp(packet->return_anchor, hchain_current(ass->sign_hash_chain), HASHSIZE) == 0) {

		*valid = true;
		// copy new anchor
		association_add_ack_element(ass, packet->anchor);
		ring_buffer_read(ass->ack_anchors);

		association_dec_sign_rounds(ass);
		association_set_state(ass, ASS_TRANS_MODE_SENDING, ASS_STATE_READY);
		association_send_s2(ass, conf);
		return AP_ERR_SUCCESS;
	} else {
		*valid = false;
		return AP_ERR_SUCCESS;
	}
} //]]

//! Generate new secrets for local SIGN and ACK chain and reset counters
/*
 * @param[in,out] ass the association
 * @return AP_ERR_SUCCESS if everything went fine, otherwise AP_ERR_NOMEM if there is not enough memory
 */
ap_err_code association_init_new_chains(association_t* ass, const config_t *conf, const association_direction_t dir) {
//[[
	assert(ass);

	hchain_free(ass->sign_hash_chain);
	hchain_free(ass->ack_hash_chain);

	// we have to distinguish between IN, OUT and BIDIRECTIONAL
	if(dir != ASS_DIRECTION_INCOMING) {
		ass->sign_hash_chain = hchain_create(&create_digest, HASHSIZE, conf->hchain_length, 0);
		if(ass->sign_hash_chain == NULL)
			return AP_ERR_NOMEM;
		// pop first element since the hash chain points initially points to NULL
		hchain_pop(ass->sign_hash_chain);
	}

	if(dir != ASS_DIRECTION_OUTGOING) {
		ass->ack_hash_chain = hchain_create(&create_digest, HASHSIZE, (conf->hchain_length / 2) + 10, 0);
		if(ass->ack_hash_chain == NULL)
			return AP_ERR_NOMEM;
		// pop first element since the hash chain points initially points to NULL
		hchain_pop(ass->ack_hash_chain);
	}

	// TODO: Workaround, because ack/sign anchors are used at several places though they're not needed
	if(dir == ASS_DIRECTION_OUTGOING) {
		ass->ack_hash_chain = hchain_create(&create_digest, HASHSIZE, 10, 0);
	} else if(dir == ASS_DIRECTION_INCOMING) {
		ass->sign_hash_chain = hchain_create(&create_digest, HASHSIZE, 10, 0);
	}

	return AP_ERR_SUCCESS;
} //]]

//! Calcuates the HMAC of a message
/*!
 * Calcuates the HMAC of a message using the current SIGN CHAIN element as the KEY.
 *
 * @param[in] ass the association for which the HMAC should be calculated (required for the SIGN CHAIN)
 * @param[in] buffer the acutal message
 * @param[in] buf_len the size of the message in bytes
 * @param[out] digest preallocated space for storing the current Hash Chain element (length is HASHSIZE)
 * @param[out] hmac_digest preallocated space for the resulting HMAC
 *
 * @return a pointer to the calculated HMAC
 *
 * @note Allocate digest and hmac_digest yourself!
 */
unsigned char* association_hash_hmac_packet(const association_t* ass, const unsigned char* buffer, const size_t buf_len, unsigned char* digest, unsigned char* hmac_digest) {
//[[
	assert(ass != NULL);
	assert(ass->sign_hash_chain != NULL);
	assert(buffer != NULL);
	assert(buf_len > 0);
	assert(digest != NULL);
	assert(hmac_digest != NULL);

	unsigned char key[HASHSIZE];
	memcpy(key, hchain_next(ass->sign_hash_chain), HASHSIZE);
	memcpy(digest, hchain_current(ass->sign_hash_chain), HASHSIZE);
	return hmac(buffer, buf_len, key, HASHSIZE, hmac_digest);
} //]]

//! Returns the value of remaining anchors in the sign chain
/*!
 * @param[in] ass the association, whose sign hash chain should be checked
 * @return number of elements in the sign hash chain
 */
size_t association_remaining_anchors_sign(const association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->sign_hash_chain != NULL);
	return ass->sign_hash_chain->position;
} //]]

//! Returns the value of remaining anchors in the ack chain
/*!
 * @param[in] ass the association, whose ack hash chain should be checked
 * @return number of elements in the ack hash chain
 */
size_t association_remaining_anchors_ack(const association_t* ass) {
//[[
	assert(ass != NULL);
	assert(ass->ack_hash_chain != NULL);
	return ass->ack_hash_chain->position;
}//]]

//! Handles timeouts for the signature state machine
/*!
 * If a timeout occures, that is related to usual data transfer (S1, A1, S2), then this function
 * checks whether all timeouts are fullfilled and retransmits packets if necessary.
 *
 * @param[in] ass the association which is checked
 * @param[in] conf the configuration to be used to resend the packet
 *
 * @return AP_ERR_SUCCESS if no
 */
ap_err_code association_handle_timeouts(association_t* ass, const config_t* conf) {
//[[
	assert(ass != NULL);

	if(association_get_state(ass, ASS_TRANS_MODE_SENDING) == ASS_STATE_SENT_S1_WAIT_A1
		&& association_get_timestamp(ass) + timemanager_get_s1_timeout(ass)  < time(NULL) ) {

		AP_MSG_F(AP_MSG_LVL_WARN, AP_MSG_CTX_ST, "<%d, %3d> No A1 after %d seconds. Retransmitting.\n", association_get_client_id(ass),
			association_get_id(ass), timemanager_get_s1_timeout(ass));

		association_set_timestamp(ass, time(NULL));
		association_send_s1(ass, conf);
	}
	return AP_ERR_SUCCESS;
} //]]


//! Gets the timeout for a given association
/*!
	The timeout is used to flush packets (i.e. if there is only one packet in the Alpha M queue and
	cannot be sent use flushing). The timeouts make sure that packets are sent eventually.

	@param[in,out] ass the association for which the timeout should be returned
	@param[out] timeout preallocated structure to store the timeout (without adding the current time)
 */
void association_get_packet_timeout(association_t* ass, struct timeval* timeout) {
//[[
	assert(ass != NULL);
	assert(timeout != NULL);
	switch(ass->mode) {
		case ALPHA_N:
			alpha_n_ass_collect_timeout((alpha_n_ass_t*)ass, timeout);
			break;
		case ALPHA_C:
			alpha_c_ass_collect_timeout((alpha_c_ass_t*)ass, timeout);
			break;
		case ALPHA_M:
			alpha_m_ass_collect_timeout((alpha_m_ass_t*)ass, timeout);
			break;
		case ALPHA_Z:
			alpha_z_ass_collect_timeout((alpha_z_ass_t*)ass, timeout);
			break;
	}
} //]]

//! Takes as many packets from the (host)queue as possible
/*!
	Associations may take different number of packets at a time.
	For example, Alpha M only likes to take \f$ 2^x \f$ packets
	out of the queue at a time. This method takes as many packets
	as possible from the host queue.

	@remark this function also might take zero packets from the queue

	@param[in,out] ass association to which packets shall be added
	@param[in,out] queue queue from which packets shall be withdrawn

	@return number of packets that have been taken out of the queue

	@see association_flush_queue()
 */
unsigned int ass_pop_packets(association_t* ass, list_t* queue) {
//[[
	assert(ass != NULL);
	assert(queue != NULL);
	switch(ass->mode) {
		case ALPHA_N:
			return alpha_n_ass_pop_packet((alpha_n_ass_t*)ass, queue);
		case ALPHA_C:
			return alpha_c_ass_pop_packet((alpha_c_ass_t*)ass, queue);
		case ALPHA_M:
			return alpha_m_ass_pop_packet((alpha_m_ass_t*)ass, queue);
		case ALPHA_Z:
			return alpha_z_ass_pop_packet((alpha_z_ass_t*)ass, queue);
	}
	return 0;
} //]]

//! Flushes a given queue
/*!
	This function allows other modules to force a assocation to transmit all packets
	that are in the given queue. Handle this function with care, use ass_pop_packets()
	in front of this function, which increases efficiency. Use this function only if
	ass_pop_packets() returns zero.

	@param[in,out] ass association to that packets shall be added
	@param[in,out] queue queue from which packets shall be withdrawn

 */
void association_flush_queue(association_t* ass, list_t* queue) {
//[[
	assert(ass != NULL);
	assert(queue != NULL);
	switch(ass->mode) {
		case ALPHA_N:
			alpha_n_ass_flush_queue((alpha_n_ass_t*)ass, queue);
			break;
		case ALPHA_C:
			alpha_c_ass_flush_queue((alpha_c_ass_t*)ass, queue);
			break;
		case ALPHA_M:
			alpha_m_ass_flush_queue((alpha_m_ass_t*)ass, queue);
			break;
		case ALPHA_Z:
			alpha_z_ass_flush_queue((alpha_z_ass_t*)ass, queue);
			break;
	}
} //]]

