// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	alpha_n.c
 * @brief	ALPHA-N related functions
 */

#include <assert.h>

#include "alpha_n.h"
#include "digest.h"
#include "tools.h"
#include "xmalloc.h"

//! Takes a given amount of packets from the queue
/*!
 * Nothing is performed if the association is not ready.
 *
 * @param[in,out] ass the association to which the packets shall be added
 * @param[in,out] queue the queue from which packets shall be taken
 * @see alpha_n_ass_flush_queue()
 */
unsigned int alpha_n_ass_pop_packet(alpha_n_ass_t* ass, list_t* queue) {
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

//! Flushes all packets from the given queue
/*!
 * Takes out all packets from the given queue and sends them immidiatly.
 *
 * @param[in] ass the alpha m association, which should be flushed
 * @param[in,out] queue the queue from which packets should be taken
 * @see alpha_n_ass_pop_packet
 */
void alpha_n_ass_flush_queue(alpha_n_ass_t* ass, list_t* queue) {
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

//! Dummy function
/*!
 * This function is needed for Alpha M and Alpha C and is still implemented by
 * association.c to provide an interface for the host.c module.
 *
 * @param[in] ass any Alpha N association
 * @param[out] preallocated timeout
 * @see alpha_m_ass_collect_timeout(), alpha_c_ass_collect_timeout()
 */
void alpha_n_ass_collect_timeout(const alpha_n_ass_t* ass, struct timeval* timeout) {
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
ap_err_code alpha_n_ass_init(alpha_n_ass_t* ass, const config_t *conf, const uint32_t id, const uint32_t client_id, const association_direction_t dir) {
//[[
	assert(ass != NULL);
	association_init((association_t*)ass, conf, id, client_id, dir);
	ass->mode = ALPHA_N;

	// ALPHA_N_RING_BUFSIZE << in defines.h
	ass->signatures = ring_buffer_new(ALPHA_N_RING_BUFSIZE, HASHSIZE);
	return AP_ERR_SUCCESS;
} //]]

//! Frees a given Alpha n association structure
/*!
 * Frees the packet queue but leaves the host_client_data_t untouched.
 * Additionally the association pointer itself is also freed.
 *
 * @param[in,out] association Pointer to a alpha N association
 */
ap_err_code alpha_n_ass_free(alpha_n_ass_t* ass) {
//[[
	if(ass == NULL) {
		return AP_ERR_SUCCESS;
	}

	ring_buffer_free(ass->signatures);
	xfree(ass);
	return AP_ERR_SUCCESS;
} //]]

//! Sends a S1 packet in ALPHA N mode
/*!
 * @param[in] conf the configuration to be used
 * @param[in] ass the association, over which the packet should be sent
 *
 * @return the size of the packet in bytes
 */
socklen_t alpha_n_ass_send_s1(const config_t *conf, const alpha_n_ass_t* ass) {
//[[
	assert(conf != NULL);
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	assert(list_size(ass->packet_queue) > 0);
	const uint32_t client_id = association_get_client_id((const association_t*)ass);

	// we only have a single presignature therefore only + hashsize
	const size_t alpha_packet_size = sizeof(alpha_packet_s1_t) + HASHSIZE;

	alpha_packet_s1_t *s1_packet = xmalloc(alpha_packet_size);
	memset(s1_packet, 0, alpha_packet_size);

	s1_packet->type = PACKET_S1;
	s1_packet->association_id = association_get_id((const association_t*)ass);
	s1_packet->signatures_count = 1;

	unsigned char* packet_presignature = (unsigned char*)s1_packet + sizeof(alpha_packet_s1_t);
	size_t packet_size;
	unsigned char* packet = list_peek_front(ass->packet_queue, &packet_size);

	unsigned char anchor[HASHSIZE];
	unsigned char presignature[HASHSIZE];

	// Create hmac of packet + hash-anchor-1 and hash-anchor
	association_hash_hmac_packet((const association_t*)ass, packet, packet_size, anchor, presignature);
	memcpy(s1_packet->anchor, anchor, HASHSIZE);
	memcpy(packet_presignature, presignature, HASHSIZE);

#if AP_MSG_LVL_VERBOSE == AP_MSG_LVL
	char tmp_anchor[2*HASHSIZE+1], tmp_hmac[2*HASHSIZE+1];
	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S1  -- anchor: %.*s..., HMAC: %.*s...\n",
		client_id,
		s1_packet->association_id,
		DIGEST_PRINTLEN, digeststr(s1_packet->anchor, tmp_anchor),
		DIGEST_PRINTLEN, digeststr(packet_presignature, tmp_hmac));
#endif

	socklen_t retval = ap_protocol_send_udp(conf, client_id, s1_packet, alpha_packet_size);
	xfree(packet);
	xfree(s1_packet);
	return retval;
} //]]

//! Handles a S1 packet in Alpha N mode
/*!
 * Copies the signature for the next packet to the association and calls necessary function
 * for hash chains from the association.h module.
 *
 * @param[in,out] ass the corresponding association
 * @param[in] packet the incoming packet
 *
 * @return AP_ERR_SUCCESS
 */
ap_err_code alpha_n_ass_handle_s1(alpha_n_ass_t* ass, const alpha_packet_s1_t* packet) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	// store the signature
	//memcpy(ass->signature, (const unsigned char*)packet + sizeof(alpha_packet_s1_t), HASHSIZE);
	ring_buffer_insert(ass->signatures, (const unsigned char*)packet + sizeof(alpha_packet_s1_t));
	return AP_ERR_SUCCESS;
} //]]

//! Sends a S2 packet
/*!
 * @param[in] conf the configuration to be used
 * @param[in,out] ass the association, over which the packet should be sent
 *
 * @return errorcode
 */
socklen_t alpha_n_ass_send_s2(const config_t* conf, alpha_n_ass_t* ass) {
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

	packet->type = PACKET_S2;
	packet->association_id = association_get_id((association_t*)ass);

	// copy the anchor for the packet
	memcpy(packet->anchor, hchain_current(ass->sign_hash_chain), HASHSIZE);

	// Copy the actual content after the end of the header
	memcpy((char*)packet + sizeof(alpha_packet_s2_t), buf, len);

	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S2  -- anchor: %.*s...\n",
		client_id, packet->association_id, DIGEST_PRINTLEN, digeststr(packet->anchor,NULL));

	int ret = ap_protocol_send_udp(conf, client_id, packet, sizeof(alpha_packet_s2_t) + len);
	xfree(buf);
	xfree(packet);

	return ret;
} //]]

//! Handles an incoming S2 packet
/*!
 * Usually this function performs some operations on the association but because this
 * association type only supports one packet, nothing (i.e. no WRITE operation)
 * is done on the association.
 *
 * @param[in] ass the association on which the packet came in
 * @param[in] packet the incoming packet
 * @param[in] payload pointer to the payload of the packet (should be in most cases (unsigned char*)packet + sizeof(alpha_packet_s2_t)
 * @param[in] payload_len length of the payload (NOT THE ENTIRE PACKET)
 * @param[out] valid if the packet is valid (output parameter!)
 * @param[out] valid pre-allocated space to store whether this packet is valid
 * @param[out] real_payload A pointer to the actual payload
 * @param[out] real_payload_size the size of the actual payload
 * @return AP_ERR_SUCCESS if everything went fine
 */
ap_err_code alpha_n_ass_handle_s2(alpha_n_ass_t* ass, const alpha_packet_s2_t* const packet, unsigned char* const payload,
	const size_t payload_len, bool* const valid, unsigned char** real_payload, size_t* const real_payload_size) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_len > 0);
	assert(valid != NULL);

	// for security issues ;)
	*valid = false;

	// DEBUG PRINTING
#if AP_MSG_LVL == AP_MSG_LVL_VERBOSE
	unsigned char temp_buf[HASHSIZE];
	hmac(payload, payload_len, packet->anchor, HASHSIZE, temp_buf);
	char anchor_out[2*HASHSIZE+1], temp_buf_out[2*HASHSIZE+1], ack_anchor_out[2*HASHSIZE+1];
	unsigned char expected_hmac[HASHSIZE];
	memset(expected_hmac, 0, HASHSIZE);
	unsigned char* expected_hmac_buf = ring_buffer_const_read(((const alpha_n_ass_t*)ass)->signatures);
	if(expected_hmac_buf != NULL) {
		memcpy(expected_hmac, expected_hmac_buf, HASHSIZE);
	}
	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Got S2      -- anchor: %.*s..., HMAC: %.*s..., expected HMAC: %.*s...\n",
		association_get_client_id((const association_t*)ass), association_get_id((const association_t*)ass),
		DIGEST_PRINTLEN, digeststr(packet->anchor, anchor_out),
		DIGEST_PRINTLEN, digeststr(temp_buf, temp_buf_out),
		DIGEST_PRINTLEN, digeststr(ring_buffer_const_read(((const alpha_n_ass_t*)ass)->signatures), ack_anchor_out)
		);
#endif
	// END DEBUG PRINTING

	unsigned char computed_hmac[HASHSIZE];
	hmac(payload, payload_len, packet->anchor, HASHSIZE, computed_hmac);
	unsigned char* stored_hmac = ring_buffer_const_read(ass->signatures);
	if(stored_hmac != NULL) {
		if(memcmp(stored_hmac, computed_hmac, HASHSIZE) == 0) {
			*valid = true;
			ring_buffer_read(ass->signatures);
		} else {
			*valid = ring_buffer_find(ass->signatures, computed_hmac);
		}
	}

	if(*valid) {
		// move the ringbuffer forward
		ring_buffer_read(ass->signatures);
		association_set_state((association_t*)ass, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);
		association_dec_ack_rounds((association_t*)ass);
	}

	*real_payload = payload;
	*real_payload_size = payload_len;
	return AP_ERR_SUCCESS;
} //]]
