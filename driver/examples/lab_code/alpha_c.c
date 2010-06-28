// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	alpha_c.c
 * @brief	ALPHA-C related functions
 */

#include <assert.h>

#include "alpha_c.h"
#include "digest.h"
#include "tools.h"
#include "xmalloc.h"

//! Takes a given amount of packets from the queue
/*!
 * The amount of packets taken from the queue is currently given
 * by the variable ALPHA_C_MINIMUM_PACKETS.
 *
 * If there are not enough packets in the supplied queue, nothing will be done.
 * Also nothing is performed if the association is not ready.
 *
 * @param[in,out] ass the association to which the packets shall be added
 * @param[in,out] queue the queue from which packets shall be taken
 * @see alpha_c_ass_flush_queue()
 */
unsigned int alpha_c_ass_pop_packet(alpha_c_ass_t* ass, list_t* queue) {
//[[
	assert(ass != NULL);
	assert(queue != NULL);

	// if we process a packet do nothing
	if(list_size(ass->packet_queue) > 0) {
		return 0;
	}

	// ugly #ifdef workaround for the GCC so this is NOT marked as warning
#if (ALPHA_C_MINIMUM_PACKETS > 0)
	if(list_size(queue) < ALPHA_C_MINIMUM_PACKETS) {
		return 0;
	}
#endif

	// take out all possible packets
	unsigned char* packet;
	size_t packet_size;
	const unsigned int packets = list_size(queue) > MAX_PRESIG_COUNT ? MAX_PRESIG_COUNT : list_size(queue);
	unsigned int i;
	for(i=0; i < packets; ++i) {
		packet = list_pop_front(queue, &packet_size);
		list_push_back(ass->packet_queue, packet, packet_size);
		xfree(packet);
	}

	return packets;
} //]]

//! Flushes the given queue
/*!
 * Takes all packets out of the queue and adds them to the association.
 * This should only be done if the packets transmitted are important or
 * it is important to empty the given queue.
 *
 * @param[in,out] ass association, to which the packets shall be added
 * @param[in,out] queue queue from which the packets are taken
 *
 * @see alpha_c_ass_pop_packet()
 */
void alpha_c_ass_flush_queue(alpha_c_ass_t* ass, list_t* queue) {
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

//! Returns the time, which the association should wait before processing new packets
/*!
 * @param[in] ass not used yet
 * @return The time which the association should wait
 */
void alpha_c_ass_collect_timeout(const alpha_c_ass_t* ass, struct timeval* timeout) {
//[[
	assert(ass != NULL);
	assert(timeout != NULL);

	if(ALPHA_C_PACKET_TIMEOUT == 0) {
		timeout->tv_sec  = 0;
		timeout->tv_usec = 0;
		return;
	}

	timeout->tv_sec = 0;
	timeout->tv_usec = (unsigned int)(((double)ALPHA_C_PACKET_TIMEOUT)/2.0 + (((double)ALPHA_C_PACKET_TIMEOUT)/2.0)*(rand() / RAND_MAX));
} //]]

//! Returns the size, the queue of this association should have before processing new packets
/*!
 * @param[in] ass not used yet
 * @return The number of packets which need to be in the queue to get packets processed
 */
static inline size_t alpha_c_ass_get_min_queue_size(const alpha_c_ass_t* ass) {
//[[
	return 0;
} //]]

//! Initializes the client data structure for alpha c
/*!
 * @param[in] ass Pointer to already allocated alpha m association data structure
 * @return Errorcode
 */
ap_err_code alpha_c_ass_init(alpha_c_ass_t* ass, const config_t *conf, const uint32_t association_id, const uint32_t client_id, const association_direction_t dir) {
//[[
	assert(ass != NULL);
	association_init((association_t*)ass, conf, association_id, client_id, dir);
	ass->mode = ALPHA_C;
	ass->packets_pre_signed = 0;
	ass->signatures = ring_buffer_new(ALPHA_C_RING_BUFSIZE, HASHSIZE);
	ass->buffer_time = time(NULL);
	return AP_ERR_SUCCESS;
} //]]

//! Frees a given association
/*!
 * Frees all data elements from a given association, except the client data.
 * @param[in,out] ass the association to be freed
 */
ap_err_code alpha_c_ass_free(alpha_c_ass_t* ass) {
//[[
	if(ass == NULL) {
		return AP_ERR_SUCCESS;
	}

	ring_buffer_free(ass->signatures);
	xfree(ass);

	return AP_ERR_SUCCESS;
} //]]

//! Sends a Alpha C S1 packet
/*!
 * @param[in] conf the configuration used to send the S1 packet
 * @param[in] ass the association used to send the S1 packet
 * @return number of sent bytes
 */
socklen_t alpha_c_ass_send_s1(const config_t* conf, alpha_c_ass_t* ass) {
//[[
	assert(conf != NULL);
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	assert(list_size(ass->packet_queue) > 0);
	const uint32_t client_id = association_get_client_id((association_t*)ass);

	// calculate macs from all packets in the queue

	// num of packets
	const uint32_t l_size = list_size(ass->packet_queue);
	const size_t packets = l_size <= MAX_PRESIG_COUNT ? l_size : MAX_PRESIG_COUNT;
	ass->packets_pre_signed = packets;
	list_iterator_t* it = list_begin(ass->packet_queue);
	const size_t alpha_packet_size = sizeof(alpha_packet_s1_t) + (packets * HASHSIZE);

	// malloc the actual packet + space for the HMACS (each is HASHSIZE long)
	alpha_packet_s1_t *s1_packet = xmalloc(alpha_packet_size);
	memset(s1_packet, 0, alpha_packet_size);
	s1_packet->type = PACKET_S1;
	s1_packet->association_id = association_get_id((association_t*)ass);

	//! \TODO not used yet!
	s1_packet->signatures_count = packets;

	// pointer set to the region where the HMACS will be stored (later used for iteration)
	unsigned char* ptr = (unsigned char*)s1_packet + sizeof(alpha_packet_s1_t);
	size_t packet_size;
	unsigned char* packet = list_iterator_get(it, &packet_size);
	unsigned char anchor[HASHSIZE];
	unsigned char presignature[HASHSIZE];

	// dont move this code into the for loop
	// because here the SIGN ANCHOR is stored in the packet!
	association_hash_hmac_packet((association_t*)ass, packet, packet_size, anchor, presignature);
	memcpy(s1_packet->anchor, anchor, HASHSIZE);
	memcpy(ptr, presignature, HASHSIZE);
	list_iterator_next(it);

	// for all other packets just append the HMAC
	for(ptr = ptr + HASHSIZE; ptr < (unsigned char*)s1_packet + alpha_packet_size; ptr = ptr + HASHSIZE) {
		packet = list_iterator_get(it, &packet_size);
		association_hash_hmac_packet((association_t*)ass, packet, packet_size, anchor, presignature);
		memcpy(ptr, presignature, HASHSIZE);
		list_iterator_next(it);
	}

	AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S1  -- anchor: %.*s...\n",
		client_id,
		s1_packet->association_id,
		DIGEST_PRINTLEN, digeststr(s1_packet->anchor,NULL));

	socklen_t ret_val = ap_protocol_send_udp(conf, client_id, s1_packet, alpha_packet_size);

	xfree(s1_packet);
	xfree(it);

	return ret_val;

} //]]

//! Handles a incoming S1 packet
/*!
 * Processes an incoming S1 packet. All signatures from the packet are stored within the
 * ring buffer. Note, that the ringbuffer could get overwritten during this operation. This
 * might cause packets which have low latency to be dropped as the signatures for these packets
 * were already overwritten.
 *
 * @param[in,out] ass the association on which the packet comes in
 * @param[in] packet pointer to the alpha header of the incoming packet
 * @param[in] payload points behind the alpha header
 * @param[in] payload_size the size of the entire packet substracted the alpha header
 *
 * @return AP_ERR_SUCCESS
 */
ap_err_code alpha_c_ass_handle_s1(alpha_c_ass_t* ass, const alpha_packet_s1_t* packet, const unsigned char* payload, const size_t payload_size) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_size > 0);

	// store data into the ring buffer

	const unsigned char* packet_ptr = payload;


	// iterate over the packets payload
	for(packet_ptr = payload; packet_ptr != (payload + payload_size); packet_ptr += HASHSIZE) {
		ring_buffer_insert(ass->signatures, packet_ptr);
	}
	ass->packets_pre_signed = payload_size / HASHSIZE;

	return AP_ERR_SUCCESS;
} //]]

//! Sends all Alpha C S2 packets
/*!
 * Sends all Alpha C S2 packet and additionally decreases the number of packets which
 * can be sent in this operation (i.e. with the previously sent S1 packet).
 *
 * @param[in] conf the configuration to be used to send the packets
 * @param[in,out] ass the association over which the packets should be sent
 */
socklen_t alpha_c_ass_send_s2(const config_t* conf, alpha_c_ass_t* ass) {
//[[
	assert(conf != NULL);
	assert(ass != NULL);
	assert(ass->packet_queue != NULL);
	assert(list_size(ass->packet_queue) > 0);
	assert(ass->packets_pre_signed > 0);
	assert(list_size(ass->packet_queue) >= ass->packets_pre_signed);
	const uint32_t client_id = association_get_client_id((association_t*)ass);

	// sent all packets for which presignatures have been sent

	// this const var is important as packets_pre_signed is decreased in each step!
	const uint32_t packets_to_send = ass->packets_pre_signed;
	uint32_t i;
	// MUST BE NULL as this variable is freed before allocating (see below)
	alpha_packet_s2_t *packet = NULL;
	unsigned int bytes_count = 0;

	/* TODO: This could be handled in paralell so that the protocol flow is not interrupted for long */
	for(i = 0; i < packets_to_send; ++i) {

		size_t len;
		unsigned char* buf = list_pop_front(ass->packet_queue, &len);
		// ok this looks weird. The idea is that
		// in the first loop a NULL pointer is freed (that is ok)
		// in the n-th loop the packet from the n-1-th loop is freed
		// => after this loop we have a valid packet variable (not freed)
		// => this can be used to get the anchor
		// => to avoid mem leaks after this loop, packet needs to be freed!
		if(packet != NULL) {
			xfree(packet);
		}
		packet = xmalloc(sizeof(alpha_packet_s2_t) + len);
		memset(packet, 0, sizeof(alpha_packet_s2_t) + len);

		packet->type = PACKET_S2;
		packet->association_id = association_get_id((association_t*)ass);
		memcpy(packet->anchor, association_get_sign_element((association_t*)ass), HASHSIZE);

		// Copy the actual content after the end of the header
		memcpy((char*)packet + sizeof(alpha_packet_s2_t), buf, len);
		xfree(buf);

		AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST, "<%d, %3d> Sending S2  -- anchor: %.*s...\n",
			client_id,
			packet->association_id,
			DIGEST_PRINTLEN, digeststr(packet->anchor,NULL));

		bytes_count += ap_protocol_send_udp(conf, client_id, packet, sizeof(alpha_packet_s2_t) + len);

		ass->packets_pre_signed--;
	}

	ass->packets_pre_signed = 0;
	ass->buffer_time = time(NULL);

	xfree(packet);

	return bytes_count;

} //]]

//! Handles an incoming S2 packet
/*!
 * Decreases the counter for packets which can be received using the list of the presignatures
 * from the last S1 packet. Removes also the latest presignature from the presignature list if
 * this packet is valid.
 *
 * @param[in] ass the association on which the packet came in
 * @param[in] packet the incoming packet
 * @param[in] payload pointer to the payload of the packet (should be in most cases (unsigned char*)packet + sizeof(alpha_packet_s2_t)
 * @param[in] payload_len length of the payload (NOT THE ENTIRE PACKET)
 * @param[out] valid if the packet is valid (output parameter!)
 * @param[out] real_payload A pointer to the actual payload
 * @param[out] real_payload_size the size of the real payload
 *
 * @return AP_ERR_SUCCESS if everything went fine
 */
ap_err_code alpha_c_ass_handle_s2(alpha_c_ass_t* const ass, const alpha_packet_s2_t* const packet,
	unsigned char* const payload, const size_t payload_len, bool* const valid, unsigned char** real_payload,
	size_t* const real_payload_size) {
//[[
	assert(ass != NULL);
	assert(packet != NULL);
	assert(payload != NULL);
	assert(payload_len > 0);
	assert(valid != NULL);

	// for security issues ;)
	*valid = false;
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
		ass->packets_pre_signed--;
		if(ass->packets_pre_signed == 0) {
			association_set_state((association_t*)ass, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);
			association_dec_ack_rounds((association_t*)ass);
		}
	}

	*real_payload = payload;
	*real_payload_size = payload_len;
	return AP_ERR_SUCCESS;
} //]]
