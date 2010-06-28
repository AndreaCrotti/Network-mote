// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	control_association.c
 * @brief	Control and use the control-association for signaling	
 */

/*!
 * @file control_association.c
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * @date SS 2009
 *
 * This file contains methods to send and handle control messages to the opposing
 * client. Control messages are embedded into the alpha protocol, meaning
 * control messages are also payload of the alpha protocol. Therefore
 * no special methods are needed to secure control messages. 
 */

#include <sys/socket.h>
#include <stdlib.h>
#include <assert.h>

#include "control_association.h"
#include "association.h"
#include "alpha_m.h"
#include "alpha_n.h"
#include "alpha_c.h"
#include "alpha_z.h"
#include "xmalloc.h"
#include "host.h"
#include "tools.h"
#include "digest.h"
#include "alpha.h"

//! Where the association id is positioned in a new association set
#define BOOTSTRAP_ASS_OFFSET_ASS_ID 0
//! Where the mode is positioned in a new association set
#define BOOTSTRAP_ASS_OFFSET_MODE (BOOTSTRAP_ASS_OFFSET_ASS_ID + sizeof(uint8_t))
//! Where the sign anchor is positioned in a new association set
#define BOOTSTRAP_ASS_OFFSET_SIGN (BOOTSTRAP_ASS_OFFSET_MODE + sizeof(uint8_t))
//! Where the ack anchor is positioned in a new association set
#define BOOTSTRAP_ASS_OFFSET_ACK (BOOTSTRAP_ASS_OFFSET_SIGN + HASHSIZE)
//! the size of one association set
#define BOOTSTRAP_ASS_OFFSET_PER_ASS (BOOTSTRAP_ASS_OFFSET_ACK + HASHSIZE)

//! Launches a request for a new control association
/*!
 * Only the node, starting the request gets a new association, the other
 * one keeps their control association. Use host_create_new_default_association()
 * to create a new control association.
 *
 * The request works like:
 * \msc
 * A,B;
 * A->B [ label="NEW ASS" ];
 * A<-B [ label="ACK" ];*
 * \endmsc
 *
 *
 * \msc
 * A,B;
 * A->B [ label="S1" ];
 * A<-B [ label="A1" ];
 * A->B [ label="S2 (NEW ASS)" ];
 * A<-B [ label="S1" ];
 * A->B [ label="A2" ];
 * A<-B [ label="S2 (ACK)" ];
 * \endmsc
 *
 * @param[in] client_id the id of the client to which the request should be sent
 * @param[in] new_def_ass pointer to a newly created control association
 * @return AP_ERR_SUCCESS
 *
 * @see host_create_new_default_association(), ca_hdl_ctrl_ass()
 */
ap_err_code ca_req_ctrl_ass(const unsigned int client_id, association_t *new_ass) {
//[[
	// send the "syn" packet to the opposite site
	const size_t header_size = sizeof(alpha_packet_new_ass_t);
	const size_t packet_size = header_size + BOOTSTRAP_ASS_OFFSET_PER_ASS;

	alpha_packet_new_ass_t *packet = xmalloc(packet_size);
	packet->type = PACKET_NEW_ASS;

	packet->num_associations = 1;
	ca_encode_ass(0, (unsigned char*)packet, header_size, 0,
		ALPHA_N, association_get_sign_element(new_ass), association_get_ack_element(new_ass), HASHSIZE);

	AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Sending SYN for new control association!\n", client_id, 0);
	association_add_packet((association_t*)host_get_default_association(client_id), (unsigned char*)packet, packet_size);
	xfree(packet);

	return AP_ERR_SUCCESS;
} //]]

//! Handles the request for a new control association
/*!
 * @param[in] client_id the id of the client on which this packet comes in
 * @param[in] packet the actual packet
 * @note the packet only may contain a SINGLE association request
 * @see ca_req_ctrl_ass
 */
static ap_err_code ca_hdl_ctrl_ass(const unsigned int client_id,
	const alpha_packet_new_ass_t* packet) {
//[[
	AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got SYN for new control association!\n", client_id, 0);
	association_t* new_association = (association_t*)host_get_default_association(client_id);

	association_t* def_ass = (association_t*)host_get_default_association(client_id);
	const size_t header_size = sizeof(alpha_packet_new_ass_t);
	unsigned char sign_anchor[HASHSIZE];
	unsigned char ack_anchor[HASHSIZE];
	ca_decode_ass(0, (const unsigned char*)packet,
		header_size, sign_anchor, ack_anchor, HASHSIZE);

	ring_buffer_insert(def_ass->sign_anchors, sign_anchor);
	ring_buffer_insert(def_ass->ack_anchors, ack_anchor);

	const size_t header_size_ack = sizeof(alpha_packet_new_ass_ack_t);
	const size_t packet_size_ack = header_size_ack + BOOTSTRAP_ASS_OFFSET_PER_ASS;

	alpha_packet_new_ass_ack_t* ack_packet = xmalloc(packet_size_ack);
	ack_packet->type = PACKET_NEW_ASS_ACK;
	ack_packet->num_associations = 1;
	ca_encode_ass(0, (unsigned char*)ack_packet, header_size_ack, 0,
		ALPHA_N, association_get_sign_element(new_association), association_get_ack_element(new_association), HASHSIZE);

	association_set_state(new_association, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);
	association_add_packet((association_t*)host_get_default_association(client_id), (unsigned char*)ack_packet, packet_size_ack);
	xfree(ack_packet);

	return AP_ERR_SUCCESS;
} //]]

//! Decodes one association, which should be bootstrapped out of a packet
/*!
 * The sign/ack elements are decoded out of the packet buffer. For all
 * other association information (id/type) use the function ca_decode_ass_metadata().
 * @param[in] position The position of the association which should be decoded out of the packet
 * @param[in,out] packet pointer to the packet (the is not modified!)
 * @param[in] header_size the size of the alpha packet header
 * @param[out] sign_element the current sign anchor of the new (bootstrapped) association (needs to be allocated already!)
 * @param[out] ack_element the current ack anchor of the new (bootstrapped) association (needs to be allocated already!)
 * @param[in] hashsize size of the sign and ack element
 * @return AP_ERR_SUCCESS
 * @see ca_encode_ass_meta_data(), ca_decode_ass(), BOOTSTRAP_ASS_OFFSET_PER_ASS
 */
ap_err_code ca_decode_ass(const size_t position, const unsigned char* packet, const size_t header_size,
	unsigned char* sign_element, unsigned char* ack_element, const size_t hashsize) {
//[[
	assert(packet != NULL);
	assert(sign_element != NULL);
	assert(ack_element != NULL);

	const unsigned char* starting_position = packet + header_size + position * BOOTSTRAP_ASS_OFFSET_PER_ASS;
	memcpy(sign_element, starting_position + BOOTSTRAP_ASS_OFFSET_SIGN, hashsize);
	memcpy(ack_element, starting_position + BOOTSTRAP_ASS_OFFSET_ACK, hashsize);
	return AP_ERR_SUCCESS;
} //]]

//! Encodes one association, which should be encapsulated into a packet
/*!
 * The id and type of bootstrapped association are encoded into the packet buffer.
 * @param[in] position The position of the association which should be encoded into the packet
 * @param[in,out] packet pointer to the packet (the is not modified!)
 * @param[in] header_size the size of the alpha packet header
 * @param[in] association_id the id of the association which should be encoded
 * @param[in] alpha_mode the mode of the association which should be encoded
 * @param[in] sign_element the current sign anchor of the new (bootstrapped) association
 * @param[in] ack_element the current ack anchor of the new (bootstrapped) association
 * @param[in] hashsize size of the sign and ack element
 * @return AP_ERR_SUCCESS
 * @see ca_decode_ass_metadata(), ca_decode_ass(), BOOTSTRAP_ASS_OFFSET_PER_ASS
 *
 * @example
 * //Bootstrapping of 3 associatons of type alpha N respectivly
 * void example() {
 *   // assume alpha_packet_t to be a struct defining a packet
 *   const size_t alpha_header_size = size(struct alpha_packet_t);
 *   const int associations_to_bootstrap = 3;
 *   stuct alpha_packet_t *packet = malloc(alpha_header_size + associations_to_bootstrap * BOOTSTRAP_ASS_OFFSET_PER_ASS);
 *   //set the header data ...
 *   //generate hash chains/associations
 *   unsigned char sign_elements[HASHSIZE * associations_to_bootstrap];
 *   unsigned char ack_elements[HASHSIZE * associations_to_bootstrap];
 *   //set sign, ack elements
 *   int i;
 *   for(i = 0; i < associations_to_bootstrap; ++i) {
 *     //i+1 due to the fact, that id = 0 is the default association and thus should not be used (only if necessary)
 *     ca_encode_ass(i, (unsigned char*)packet, alpha_header_size, i+1, ALPHA_N, sign_elements + i*HASHSIZE, ack_elements + i*HASHSIZE, HASHSIZE);
 *   }
 * }
 */
ap_err_code ca_encode_ass(const size_t position, unsigned char* packet, const size_t header_size, const uint8_t association_id,
	const uint8_t alpha_mode, const unsigned char* sign_element, const unsigned char* ack_element, const size_t hashsize) {
//[[
	assert(packet != NULL);
	assert(alpha_mode == ALPHA_N || alpha_mode == ALPHA_C || alpha_mode == ALPHA_M || alpha_mode == ALPHA_Z);
	assert(sign_element != NULL);
	assert(ack_element != NULL);

	unsigned char* insert_position = packet + header_size + position * BOOTSTRAP_ASS_OFFSET_PER_ASS;
	*((uint8_t*)(insert_position + BOOTSTRAP_ASS_OFFSET_ASS_ID)) = association_id;
	*((uint8_t*)(insert_position + BOOTSTRAP_ASS_OFFSET_MODE)) = alpha_mode;
	memcpy(insert_position + BOOTSTRAP_ASS_OFFSET_SIGN, sign_element, hashsize);
	memcpy(insert_position + BOOTSTRAP_ASS_OFFSET_ACK, ack_element, hashsize);
	return AP_ERR_SUCCESS;
} //]]

//! Decodes one association, which should be extracted from a packet
/*!
 * The id and type of bootstrapped association are decoded out of the packet buffer. For all
 * other association information (sign/ack elements) use the function ca_decode_ass().
 * This only gets you the important stuff.
 * @param[in] position The position of the association which should be decoded out of the packet
 * @param[in,out] packet pointer to the packet (the is not modified!)
 * @param[in] header_size the size of the alpha packet header
 * @param[out] association_id the id of the association which is decoded
 * @param[out] alpha_mode the mode of the association which is decoded
 * @return AP_ERR_SUCCESS
 * @see ca_encode_ass(), ca_decode_ass(), BOOTSTRAP_ASS_OFFSET_PER_ASS
 */
ap_err_code ca_decode_ass_metadata(const size_t position, const unsigned char* packet, const size_t header_size,
	uint8_t* const association_id, uint8_t* const alpha_mode) {
//[[
	assert(packet != NULL);
	assert(association_id != NULL);
	assert(alpha_mode != NULL);

	const unsigned char* starting_position = packet + header_size + position * BOOTSTRAP_ASS_OFFSET_PER_ASS;
	*association_id = *((const uint8_t*)(starting_position + BOOTSTRAP_ASS_OFFSET_ASS_ID));
	*alpha_mode = *((const uint8_t*)(starting_position + BOOTSTRAP_ASS_OFFSET_MODE));
	return AP_ERR_SUCCESS;
} //]]

//! Handles kill requests for associations
/*!
 * Handle incoming kill requests for associations. If local outgoing ass are 
 * killed then the same amount of new ones are spawned.
 *
 * @param[in] client_id the id of the client on which the packet came in
 * @param[in] packet the control packet which comes in
 * @param[in] packet_len the length of the control packet
 * @return the value of the ca_request_new_asss() function
 */
static ap_err_code ca_hdl_kill_ass(const unsigned int client_id, const config_t* conf,
	const alpha_packet_kill_ass_t* packet, const unsigned int packet_len) {
//[[ 
	assert(packet != NULL);
	assert(packet_len > 0);

	const unsigned char* payload = (const unsigned char*)packet + sizeof(alpha_packet_kill_ass_t);
	const unsigned int payload_len = packet_len - sizeof(alpha_packet_kill_ass_t);
	typedef uint8_t ass_id;

	unsigned int i;
	const unsigned int ass_count = payload_len / sizeof(ass_id);
	association_t* cur_ass;
	//unsigned char* packet_queued = NULL;
	//size_t packet_queued_len = 0;
	
	for(i = 0; i < ass_count; ++i) {
		const uint8_t *cur_id = (const uint8_t *) (payload + i * sizeof(ass_id));
		const unsigned int id = (const unsigned int) *cur_id;

		cur_ass = host_get_incoming_association(client_id, id);

		if(cur_ass) {
			AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got KILL for IN association %d\n", client_id, 0, id);
		} else {
			AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got KILL for IN association %d, but could not find it\n", client_id, 0, id);
			continue;
		}

		//while(list_size(cur_ass->packet_queue) > 0) {
		//	packet_queued = list_pop_front(cur_ass->packet_queue, &packet_queued_len);
		//	host_enqueue_packet(client_id, packet_queued, packet_queued_len);
		//	xfree(packet_queued);
		//}

		host_remove_association(client_id, id, ASS_DIRECTION_INCOMING);
	}

	return AP_ERR_SUCCESS;
} //]]

//! Processes an incoming control packet which states to create a new association
/*!
 * @param[in] client_id the position of the client in the clients array who queried to create a new association
 * @param[in] conf the configuration used to sent the control packets
 * @param[in] packet the incoming alpha control packet
 * @param[out] valid preallocated space to store whether the signatures of the packet are valid
 *
 * Sends an ok message and creates a new association, if the anchors are valid.
 *
 * \see ca_request_new_asss
 */
static ap_err_code ca_hdl_new_ass(const unsigned int client_id, const config_t* conf,
	const alpha_packet_new_ass_t* packet) {
//[[
	assert(packet != NULL);

	const size_t header_size = sizeof(alpha_packet_new_ass_t);

	// send ack packet
	const size_t header_size_ack = sizeof(alpha_packet_new_ass_ack_t);
	const size_t packet_size_ack = header_size_ack + BOOTSTRAP_ASS_OFFSET_PER_ASS * packet->num_associations;

	uint8_t new_ass_id = 0;
	uint8_t mode = 0;

	// check if packet asks for a new default association
	ca_decode_ass_metadata(0, (const unsigned char*)packet, header_size, &new_ass_id, &mode);

	if(new_ass_id == 0) {
		return ca_hdl_ctrl_ass(client_id, packet);
	}

	alpha_packet_new_ass_ack_t* ack_packet = xmalloc(packet_size_ack);
	ack_packet->type = PACKET_NEW_ASS_ACK;
	ack_packet->num_associations = 0;

	association_t* new_association = NULL;

	unsigned int i;
	for(i = 0; i < packet->num_associations; ++i) {

		ca_decode_ass_metadata(i, (const unsigned char*)packet, header_size, &new_ass_id, &mode);

		if(host_get_incoming_association(client_id, new_ass_id)) {
			continue;
		}

		switch(mode) {
			case ALPHA_N:
				{
					new_association = xmalloc(sizeof(alpha_n_ass_t));
					if(alpha_n_ass_init((alpha_n_ass_t*)new_association, conf, new_ass_id, client_id, ASS_DIRECTION_INCOMING) != AP_ERR_SUCCESS) {
						xfree(new_association);
						return 0;
					}
					AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got SYN for new IN association %d (N)\n", client_id, 0, new_ass_id);
					break;
				}
			case ALPHA_C:
				{
					new_association = xmalloc(sizeof(alpha_c_ass_t));
					if(alpha_c_ass_init((alpha_c_ass_t*)new_association, conf, new_ass_id, client_id, ASS_DIRECTION_INCOMING) != AP_ERR_SUCCESS) {
						xfree(new_association);
						return 0;
					}
					AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got SYN for new IN association %d (C)\n", client_id, 0, new_ass_id);
					break;
				}
			case ALPHA_M:
				{
					new_association = xmalloc(sizeof(alpha_m_ass_t));
					if(alpha_m_ass_init((alpha_m_ass_t*)new_association, conf, new_ass_id, client_id, ASS_DIRECTION_INCOMING, conf->alpha_m_sec_mode) != AP_ERR_SUCCESS) {
						xfree(new_association);
						return 0;
					}
					AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got SYN for new IN association %d (M)\n", client_id, 0, new_ass_id);
					break;
				}
			case ALPHA_Z:
				{
					new_association = xmalloc(sizeof(alpha_z_ass_t));
					if(alpha_z_ass_init((alpha_z_ass_t*)new_association, conf, new_ass_id, client_id, ASS_DIRECTION_INCOMING) != AP_ERR_SUCCESS) {
						xfree(new_association);
						return 0;
					}
					AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got SYN for new IN association %d (Z)\n", client_id, 0, new_ass_id);
					break;
				}
			default:
				xfree(new_association);
				return 0;
		}
		unsigned char sign_anchor[HASHSIZE];
		unsigned char ack_anchor[HASHSIZE];
		ca_decode_ass(i, (const unsigned char*)packet, header_size, sign_anchor, ack_anchor, HASHSIZE);
		association_add_sign_element(new_association, sign_anchor);
		association_add_ack_element(new_association, ack_anchor);
		if(host_add_association(client_id, new_association) != AP_ERR_SUCCESS) {
			return 0;
		}

		association_set_state(new_association, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_NEW);
		association_set_state(new_association, ASS_TRANS_MODE_SENDING, ASS_STATE_NEW);
		association_set_state(new_association, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);

		ca_encode_ass(ack_packet->num_associations, (unsigned char*)ack_packet,
			header_size_ack, association_get_id(new_association), mode, association_get_sign_element(new_association),
			association_get_ack_element(new_association), HASHSIZE);

 		ack_packet->num_associations++;
		hchain_pop(new_association->ack_hash_chain);

	}

	// socklen_t retval = ap_protocol_send_udp(config, client_id, ack_packet, packet_size_ack);
	host_enqueue_control_packet(client_id, (unsigned char*)ack_packet, packet_size_ack);
	xfree(ack_packet);
	// host_control_ass_set_state(association_get_client_id((association_t*)def_ass), CONTROL_ASS_READY);

	return AP_ERR_SUCCESS;
} //]]

//! Handles a ok message for creating a new association
/*!
 * Checks the signatures of the incoming packet and if these are valid a new association is created.
 *
 * @param[in] client_id the id of the client in the clients array
 * @param[in] packet the incoming alpha control packet
 * @param[out] valid preallocated space to store whether the signatures of the packet are valid
 *
 * @return AP_ERR_SUCCESS or AP_ERR_NOMEM if the association cannot be created
 */
static ap_err_code ca_hdl_new_ass_ack(const unsigned int client_id,
	const alpha_packet_new_ass_ack_t* packet) {
//[[
	assert(packet != NULL);

	uint8_t new_ass_id;
	uint8_t mode;
	const size_t header_size = sizeof(alpha_packet_new_ass_ack_t);
	association_t* def_ass = (association_t*)host_get_default_association(client_id);

	unsigned int i = 0;
	unsigned int j = 0;
	for(i = 0; i < packet->num_associations; ++i) {
		ca_decode_ass_metadata(i, (const unsigned char*)packet, header_size, &new_ass_id, &mode);

		association_t* new_ass = NULL;
		association_t* new_def_ass = NULL;
		if(new_ass_id == 0) {
			AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got ACK for new control association!\n", client_id, 0);
			new_def_ass = (association_t*)host_get_new_default_association(client_id);

			ring_buffer_copy(new_def_ass->sign_anchors, def_ass->sign_anchors);
			ring_buffer_copy(new_def_ass->ack_anchors, def_ass->ack_anchors);

			host_update_default_association(client_id);
			new_ass = (association_t*)host_get_default_association(client_id);
			host_set_state(client_id, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);
			host_set_state(client_id, ASS_TRANS_MODE_SENDING, ASS_STATE_READY);
			return AP_ERR_SUCCESS;
		} else {
			new_ass = host_get_outgoing_association(client_id, new_ass_id);
		}

		if(new_ass == NULL) {
			return AP_ERR_STATE;
		}

		// set the new association to ready
		for(j = 0; j < ASS_TRANS_MODE_COUNT; ++j) {
			association_set_state(new_ass, j, ASS_STATE_READY);
		}
		AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got ACK for new OUT association %d\n", client_id, 0, new_ass_id);

		unsigned char sign_anchor[HASHSIZE];
		unsigned char ack_anchor[HASHSIZE];
		ca_decode_ass(i, (const unsigned char*)packet, header_size, sign_anchor, ack_anchor, HASHSIZE);

		association_add_sign_element(new_ass, sign_anchor);
		association_add_ack_element(new_ass, ack_anchor);

		hchain_pop(new_ass->sign_hash_chain);

		if(new_ass_id != 0) {
			host_insert_ready_association(client_id, new_ass);
		}
	}
	return AP_ERR_SUCCESS;
} //]]

//! Checks whether the packet is a control packet
/*!
 * Currently only control packets are allowed on the default/control association
 * so all packets on that association are considered control packets. This
 * function can be used to determine if packets need to be forwarded to
 * the alpha daemon (and be processed) or if the packet is just payload
 * and can be processed by other applications.
 *
 * @param[in] packet the packet to be checked
 * @return true if the packet is a control packet
 * @see ca_hdl_ctrl_packet()
 */
inline bool ca_is_ctrl_packet(const alpha_packet_s2_t* packet) {
//[[
	return (packet->association_id == 0);
} //]]

//! Handles an incoming alpha control packet
/*!
 * Alpha control packets are embedded into alpha packets(s1, a1, s2 scheme). This
 * functions checks which type of control packet is given and calls the necessary
 * functions for further process the request.
 *
 * @param[in] client_id the id of the client on which the packet came in
 * @param[in] conf the configuration used
 * @param[in] packet the acutal control packet (without the S2 header!)
 * @param[in] packet_len the length of the control packet (without the S2 header!)
 * @return AP_ERR_SUCCESS or AP_ERR_INVALID_CONTROL_TYPE if the packet is corrupted
 * @see control_association.c
 */
ap_err_code ca_hdl_ctrl_packet(const unsigned int client_id, const config_t* conf, const unsigned char* packet, const size_t packet_len) {
//[[
	const alpha_control_packet_t* p = (const alpha_control_packet_t*)packet;
	switch(p->type) {
		case PACKET_NEW_ASS:
			ca_hdl_new_ass(client_id, conf, (const alpha_packet_new_ass_t*)packet);
			break;

		case PACKET_NEW_ASS_ACK:
			ca_hdl_new_ass_ack(client_id, (const alpha_packet_new_ass_ack_t*)packet);
			break;

		case PACKET_ASS_DIE:
			ca_hdl_kill_ass(client_id, conf, (const alpha_packet_kill_ass_t*) packet, packet_len);
			break;
		default:
			return AP_ERR_INVALID_CONTROL_TYPE;
	}
	return AP_ERR_SUCCESS;
} //]]

//! Starts to send control messages to create a new association
/*!
 * @param[in] client_id the position of the client in the clients array to with which the association should be created
 * @param[in] conf the configuration used to sent the control packets
 *
 * \msc
 * A,B;
 * A->B[label="new association, hash chain element (sign)"];
 * A<-B[label="ok, received hash chain element from A, hash chain element (ack)"]
 * \endmsc
 *
 * \see control_ass_handle_new_association()
 *
 * @return number of bytes sent
 */
ap_err_code ca_request_new_asss(const unsigned int client_id, const config_t* conf, const size_t alpha_n_connections,
	const size_t alpha_c_connections, const size_t alpha_m_connections, const size_t alpha_z_connections) {
//[[
	const size_t num_connections = alpha_n_connections + alpha_c_connections + alpha_m_connections + alpha_z_connections;

	// create the new associations
	association_t* new_ass = NULL;

	const size_t header_size = sizeof(alpha_packet_new_ass_t);
	const size_t packet_size = header_size + BOOTSTRAP_ASS_OFFSET_PER_ASS * num_connections;

	alpha_packet_new_ass_t *packet = xmalloc(packet_size);
	packet->type = PACKET_NEW_ASS;

	unsigned int i;
	unsigned int new_id;
	alpha_mode_t current_mode;

	packet->num_associations = 0;

	for(i = 0; i < num_connections; ++i) {
		if(i < alpha_n_connections) {
			current_mode = ALPHA_N;
		} else {
			if(i < alpha_n_connections + alpha_c_connections) {
				current_mode = ALPHA_C;
			} else {
				if(i < alpha_n_connections + alpha_c_connections + alpha_m_connections) {
					current_mode = ALPHA_M;
				} else {
					current_mode = ALPHA_Z;
				}
			}
		}
		switch(current_mode) {
			case ALPHA_N:
				new_ass = xmalloc(sizeof(alpha_n_ass_t));
				break;
			case ALPHA_C:
				new_ass = xmalloc(sizeof(alpha_c_ass_t));
				break;
			case ALPHA_M:
				new_ass = xmalloc(sizeof(alpha_m_ass_t));
				break;
			case ALPHA_Z:
				new_ass = xmalloc(sizeof(alpha_z_ass_t));
				break;
		}
		new_id = host_get_new_ass_id(client_id);
		switch(current_mode) {
			case ALPHA_N:
				if(alpha_n_ass_init((alpha_n_ass_t*)new_ass, conf, new_id, client_id, ASS_DIRECTION_OUTGOING) != AP_ERR_SUCCESS) {
					xfree(new_ass);
					return 0;
				}
				AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Going to request new OUT association %d (N)\n", client_id, 0, new_id);
				break;
			case ALPHA_M:
				if(alpha_m_ass_init((alpha_m_ass_t*)new_ass, conf, new_id, client_id, ASS_DIRECTION_OUTGOING, conf->alpha_m_sec_mode) != AP_ERR_SUCCESS) {
					xfree(new_ass);
					return 0;
				}
				AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Going to request new OUT association %d (M)\n", client_id, 0, new_id);
				break;
			case ALPHA_C:
				if(alpha_c_ass_init((alpha_c_ass_t*)new_ass, conf, new_id, client_id, ASS_DIRECTION_OUTGOING) != AP_ERR_SUCCESS) {
					xfree(new_ass);
					return 0;
				}
				AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Going to request new OUT association %d (C)\n", client_id, 0, new_id);
				break;
			case ALPHA_Z:
				if(alpha_z_ass_init((alpha_z_ass_t*)new_ass, conf, new_id, client_id, ASS_DIRECTION_OUTGOING) != AP_ERR_SUCCESS) {
					xfree(new_ass);
					return 0;
				}
				AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Going to request new OUT association %d (Z)\n", client_id, 0, new_id);
		}
		if(host_add_association(client_id, new_ass) != AP_ERR_SUCCESS) {
			xfree(new_ass);
			continue;
		}
		ca_encode_ass(packet->num_associations, (unsigned char*)packet, header_size, new_id, current_mode, association_get_sign_element(new_ass), association_get_ack_element(new_ass), HASHSIZE);
		packet->num_associations++;
	}

	//socklen_t retval = ap_protocol_send_udp(config, client_id, packet, packet_size);
	host_enqueue_control_packet(client_id, (unsigned char*)packet, packet_size);
	xfree(packet);

	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending request for new OUT associations: %d (N), %d (M), %d (C), %d (Z)\n", client_id, 0, 
		alpha_n_connections, alpha_m_connections, alpha_c_connections, alpha_z_connections); 

	return AP_ERR_SUCCESS;
} //]]

//! Requests a association to die
/*!
 * This is used to send a kill-request for an association to the peer. It works 
 * for outgoing and incoming associations alike. When killing local outgoing 
 * ass, it makes sure the same amount of new ones are spawned. 
 *
 * @param[in] client_id the id of the client on which this packet came in
 * @param[in] association_ids Pointer to a array of incoming association ids which shall die
 * @param[in] associations_count size of the association_ids array
 * @return AP_ERR_SUCCESS
 */
ap_err_code ca_kill_ass(const unsigned int client_id, const config_t *conf, const unsigned int *association_ids,
	const unsigned int associations_count) {
//[[
	assert(association_ids != NULL);
	assert(associations_count > 0);

	unsigned int new_alpha_n_ass = 0, new_alpha_m_ass = 0, new_alpha_c_ass = 0, new_alpha_z_ass = 0;
	association_t * cur_ass;
	unsigned int i = 0;

	for(i = 0; i < associations_count; ++i) {
		cur_ass = host_get_outgoing_association(client_id, association_ids[i]);
		if(cur_ass) {
			AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending KILL for OUT association %d\n", client_id, 0, association_ids[i]);
			switch(association_get_mode(cur_ass)) {
				case ALPHA_N:
					new_alpha_n_ass++;
					break;
				case ALPHA_M:
					new_alpha_m_ass++;
					break;
				case ALPHA_C:
					new_alpha_c_ass++;
					break;
				case ALPHA_Z:
					new_alpha_z_ass++;
					break;
				default:
					return AP_ERR_INVALID_MODE;
			}
		}
		host_remove_association(client_id, association_ids[i], ASS_DIRECTION_OUTGOING);
	}

	const unsigned int packet_size = sizeof(alpha_packet_kill_ass_t) + sizeof(uint8_t) * associations_count;
	alpha_packet_kill_ass_t *packet = xmalloc(packet_size);
	packet->type = PACKET_ASS_DIE;
	unsigned char* ptr = (unsigned char*)packet + sizeof(alpha_packet_kill_ass_t);
	for(i = 0; i < associations_count; ++i) {
		*(uint8_t*)(ptr + i*sizeof(uint8_t)) = association_ids[i];
	}
	host_enqueue_control_packet(client_id, (unsigned char*)packet, packet_size);
	xfree(packet);

	if(new_alpha_n_ass + new_alpha_c_ass + new_alpha_m_ass + new_alpha_z_ass) {
		return ca_request_new_asss(client_id, conf, new_alpha_n_ass, new_alpha_c_ass, new_alpha_m_ass, new_alpha_z_ass);
	}
	return AP_ERR_SUCCESS;
} //]]
