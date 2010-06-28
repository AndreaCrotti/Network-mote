// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include "alpha_z.h"

#include <assert.h>
#include "xmalloc.h"
#include "defines.h"
#include "tools.h"

//! This function takes the packets packets from the queue when they are distributed
/*!
 * @param[in,out] ass The association to which packets shall be added
 * @param[in,out] queue The queue from which the packets shall be taken (they are taken, NOT copied)
 *
 * @return The number of packets which are taken from the queue
 */
unsigned int alpha_z_ass_pop_packet(alpha_z_ass_t* ass, list_t* queue) {
//[[

	assert(ass != NULL);
	assert(queue != NULL);

	// if we process a packet do nothing
	if(list_size(ass->packet_queue) > 0) {
		return 0;
	}

	// take out all packets
	unsigned char* packet;
	size_t packet_size;
	const unsigned int packets = list_size(queue);

	while(list_size(queue) > 0) {
		packet = list_pop_front(queue, &packet_size);
		list_push_back(ass->packet_queue, packet, packet_size);
		xfree(packet);
	}

	return packets;
} //]]

//! This functions takes all packets from the queue and tries to send them
/*!
 * @param[in,out] ass the association to which the packets shall be added
 * @param[in,out] queue the queue from which all packets are taken
 */
void alpha_z_ass_flush_queue(alpha_z_ass_t* ass, list_t* queue) {
//[[
	assert(ass != NULL);
	assert(queue != NULL);

	// take all packets out of the queue no matter what
	unsigned char* packet;
	size_t packet_size;

	while(list_size(queue) > 0) {
		packet = list_pop_front(queue, &packet_size);
		list_push_back(ass->packet_queue, packet, packet_size);
		xfree(packet);
	}
} //]]

//! Gets the timeout for the association
/*!
 * This timeout specifies how long a association may wait for packets before flushing
 *
 * @param[in] ass the association whose timeout should be returned
 * @param[out] timeout the timeout for the given association
 */
void alpha_z_ass_collect_timeout(const alpha_z_ass_t* ass, struct timeval* timeout) {
//[[
	assert(ass != NULL);
	assert(timeout != NULL);
	timeout->tv_sec = 0;
	timeout->tv_usec = 0;
} //]]

//! Initializes the association structure for alpha n
/*!
 * @param[in,out] association Pointer to already allocated alpha n client data structure
 * @param[in] id The identifier for this association
 * @param[in] client_id the id of the client in the clients array
 * @param[in] dir the direction of the association
 */
ap_err_code alpha_z_ass_init(alpha_z_ass_t* ass, const config_t *conf, const uint32_t id, const uint32_t client_id, const association_direction_t dir) {
//[[
	assert(ass != NULL);
	association_init((association_t*)ass, conf, id, client_id, dir);
	ass->mode = ALPHA_Z;
	return AP_ERR_SUCCESS;
} //]]

//! Frees a given Alpha n association structure
/*!
 * Frees the packet queue but leaves the host_client_data_t untouched.
 * Additionally the association pointer itself is also freed.
 *
 * @param[in,out] association Pointer to a alpha N association
 */
ap_err_code alpha_z_ass_free(alpha_z_ass_t* ass) {
//[[
	if(ass == NULL) {
		return AP_ERR_SUCCESS;
	}
	xfree(ass);
	return AP_ERR_SUCCESS;
} //]]

//! Sends a S2 packet
/*!
 * @param[in] conf the configuration to be used
 * @param[in,out] ass the association, over which the packet should be sent
 *
 * @return errorcode
 */
socklen_t alpha_z_ass_send_s2(const config_t* conf, alpha_z_ass_t* ass) {
//[[
	assert(conf != NULL);
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	assert(ass->sign_hash_chain != NULL);
	assert(ass->ack_hash_chain != NULL);
	assert(list_size(ass->packet_queue) > 0);

	const uint32_t client_id = association_get_client_id((association_t*)ass);
	size_t len;
	unsigned char* buf = list_pop_front(ass->packet_queue, &len);
	const size_t total_s2_packet_size = sizeof(alpha_packet_s2_t) + len;
	alpha_packet_s2_t *packet = xmalloc(total_s2_packet_size);
	memset(packet, 0, total_s2_packet_size);

	if(packet == NULL) {
		return AP_ERR_NOMEM;
	}

	packet->type = PACKET_ALPHA_Z;
	packet->association_id = association_get_id((association_t*)ass);

	// Copy the actual content after the end of the header
	memcpy((char*)packet + sizeof(alpha_packet_s2_t), buf, len);

	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S2  -- (ALPHA Z)\n",
		client_id, packet->association_id);

	int ret = ap_protocol_send_udp(conf, client_id, packet, sizeof(alpha_packet_s2_t) + len);
	xfree(buf);
	xfree(packet);

	return ret;
} //]]

//! Handles an incoming S2 packet
/*!
 * Usually this function performs some operations on the association but as this
 * association type only supports one packet nothing (i.e. no WRITE operations)
 * is done on the association.
 *
 * @param[in] ass the association on which the packet came in
 * @param[in] packet the incoming packet
 * @param[in] payload pointer to the payload of the packet (should be in most cases (unsigned char*)packet + sizeof(alpha_packet_s2_t)
 * @param[in] payload_len length of the payload (NOT THE ENTIRE PACKET)
 * @param[out] valid if the packet is valid (output parameter!)
 * @param[out] valid pre-allocated space to store wheter this packet is valid
 * @param[out] real_payload A pointer to the actual payload
 * @param[out] real_payload_size the size of the real payload
 * @return AP_ERR_SUCCESS if everything went fine
 */
ap_err_code alpha_z_ass_handle_s2(alpha_z_ass_t* const ass, const alpha_packet_s2_t* const packet, unsigned char* payload,
	const size_t payload_len, bool* const valid, unsigned char** real_payload, size_t* const real_payload_size) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_len > 0);
	assert(valid != NULL);

	//we accept any packet
	*valid = true;

	// DEBUG PRINTING
	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got S2 (ALPHA Z)\n",
		association_get_client_id((association_t*)ass), association_get_id((association_t*)ass));
	// END DEBUG PRINTING

	*real_payload = payload;
	*real_payload_size = payload_len;
	return AP_ERR_SUCCESS;
} //]]
