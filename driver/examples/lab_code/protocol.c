// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	protocol.c
 * @brief	Handling of packets in- and outside the state-machine
 */

#include <errno.h>
#include <arpa/inet.h>
#include <assert.h>

#include "tools.h"
#include "digest.h"
#include "application.h"
#include "association.h"
#include "control_association.h"

#ifdef DEBUG_TUN
 #include <netdb.h>
 #include <netinet/ip.h>
#endif

/** Send a UDP packet to a known host
 * @param	conf		Configuration-Pointer
 * @param	client_id	Client to send to
 * @param	buf		Data to send
 * @param	len		Size of data
 * @return	Bytes actually sent (-1 on error)
 */
socklen_t ap_protocol_send_udp(const config_t *conf, const int client_id, const void *buf, const size_t len) {
//[[
	assert(buf != NULL);
	assert(len > 0);

	struct sockaddr_in to;
	socklen_t tolen = sizeof(struct sockaddr_in);

	to.sin_family = AF_INET;
	to.sin_port = htons(conf->port);
	to.sin_addr = host_get_addr(client_id);

#ifdef DEBUG_NET
	statusmsg("<%d> Sending packet (type %u, %d bytes) to %s:%d\n", client_id, ((const alpha_packet_t*)buf)->type,
		len, inet_ntoa(to.sin_addr), conf->port);
#endif

	ssize_t bytes = sendto(conf->socket_incoming, buf, len, 0, (struct sockaddr*)&to, tolen);

	if(bytes == -1) {
		print_error("sendto(): %s\n", strerror(errno));
		return -1;
	}

	return bytes;
} //]]

/* CONNECT procedure:
 *
 * A ------ CONNECT ------> B
 * A <-- RETURN_CONNECT --- B (B stateless, for IP verification, contains addr_hash)
 * A -------- SYN --------> B (using addr_hash from RETURN_CONNECT, including a challenge and anchors)
 * A <------- ACK --------- B (only now is state at B created, ACK sent with hash of challenge/anchors) 
 * A ------ ACKACK -------> B 
 *
 */

/** Send the initial CONNECT to a known host
 * @param	conf		Configuration-Pointer
 * @param	client_id	Client to send to
 */
socklen_t ap_protocol_send_connect(const config_t *conf, const int client_id) {
 //[[
	alpha_packet_connect_t packet;
	memset(&packet, 0, sizeof(alpha_packet_connect_t));
	packet.type = PACKET_CONNECT;

#ifdef DEBUG_HANDSHAKE
	statusmsg("<%d> Sending CONNECT\n", client_id);
#endif
	host_set_state(client_id, 0, ASS_STATE_SENT_CONNECT_WAIT_RETURN_CONNECT);
	if(!host_get_pubkey(client_id)) {
		statusmsg("<%d> We do not have a DSA pubkey for this host, we can't initiate a CONNECT!\n", client_id);
		host_set_state(client_id, 0, ASS_STATE_NEW);
		return 0;
	}
	return ap_protocol_send_udp(conf, client_id, &packet, sizeof(alpha_packet_connect_t));
} //]]

/** Send the RETURN_CONNECT (stateless)
 * @param	conf		Configuration-Pointer
 * @param	fromaddr	Address to send to
 * @param	fromlen		Size of address
 */
void ap_protocol_handle_connect(const config_t *conf, const struct sockaddr_in *fromaddr, const socklen_t *fromlen) {
//[[
	alpha_packet_return_connect_t packet;
	memset(&packet, 0, sizeof(alpha_packet_return_connect_t));

	packet.type = PACKET_RETURN_CONNECT;
	unsigned char addr_hash[HASHSIZE];
	memcpy(packet.addr_hash, connect_hash(conf->connect_secret, fromaddr->sin_addr, timeslot(0), addr_hash), HASHSIZE);

#ifdef DEBUG_HANDSHAKE
	statusmsg("<?> Sending RETURN_CONNECT to host %s with connect_hash %.*s...\n", inet_ntoa(fromaddr->sin_addr), DIGEST_PRINTLEN, digeststr(packet.addr_hash, NULL));
#endif

	sendto(conf->socket_incoming, &packet, sizeof(alpha_packet_return_connect_t), 0, (const struct sockaddr *)fromaddr, *fromlen);
} //]]

/** Send SYN with information from RETURN_CONNECT
 * @param	conf		Configuration-Pointer
 * @param	client_id	Client to send to
 */
socklen_t ap_protocol_send_syn(const config_t *conf, const int client_id) {
//[[
	alpha_packet_syn_t packet;
	memset(&packet, 0, sizeof(alpha_packet_syn_t));
	packet.type = PACKET_SYN;

	// Send our anchors for the opposing client
	host_generate_anchor(client_id, CHAIN_SIGN, packet.sign_anchor);
	host_generate_anchor(client_id, CHAIN_ACK, packet.ack_anchor);

	packet.challenge = htonl(host_get_challenge(client_id));

	memcpy(packet.addr_hash, host_get_addr_hash(client_id), HASHSIZE);

#ifdef DEBUG_HANDSHAKE
//[[

#ifdef DEBUG_DIGEST
	char signbuf[2*HASHSIZE+1], ackbuf[2*HASHSIZE+1];
#endif
	statusmsg("<%d> Sending SYN"
#ifdef DEBUG_DIGEST
		" --   SIGN: %.*s...,  ACK: %.*s..."
#endif
		"\n", client_id
#ifdef DEBUG_DIGEST
		, DIGEST_PRINTLEN, digeststr(packet.sign_anchor, signbuf), DIGEST_PRINTLEN, digeststr(packet.ack_anchor, ackbuf)
#endif
	);
#endif
//]]

	return ap_protocol_send_udp(conf, client_id, &packet, sizeof(alpha_packet_syn_t));
} //]]

/** Verify SYN packet (stateless)
 * @param	conf		Configuration-Pointer
 * @param	packet		SYN-packet
 * @param	fromaddr	Address SYN was received from
 */
int ap_protocol_verify_syn(const config_t *conf, const alpha_packet_syn_t *packet, const struct sockaddr_in *fromaddr) {
//[[
	unsigned char digest[HASHSIZE];

	if(memcmp(packet->addr_hash, connect_hash(conf->connect_secret, fromaddr->sin_addr, timeslot(0), digest), HASHSIZE)) {
		if(memcmp(packet->addr_hash, connect_hash(conf->connect_secret, fromaddr->sin_addr, timeslot(-TIMESLOT_SIZE), digest), HASHSIZE)) {
#ifdef DEBUG_HANDSHAKE
			print_error("Host %s sent an incorrect address hash %.*s, ignoring it\n", inet_ntoa(fromaddr->sin_addr), DIGEST_PRINTLEN, digeststr(packet->addr_hash, NULL));
#endif
			return 0;
		}
	}

	return 1;
} //]]

/** Create timeslot for use in RETURN_CONNECT
 * @param	offset	quick adjustment of timeslot (seconds)
 */
int timeslot(int offset) {
//[[
	int now = (int) time(NULL);
	int return_timeslot = now - offset - (now%TIMESLOT_SIZE);
	return return_timeslot;
} //]]

/** Send ACK handshake packet
 * @param	conf		Configuration-Pointer
 * @param	client_id	Client to send to
 */
socklen_t ap_protocol_send_ack(const config_t *conf, const int client_id) {
//[[
	alpha_packet_ack_t packet;
	memset(&packet, 0, sizeof(alpha_packet_ack_t));
	packet.type = PACKET_ACK;

	// Get our anchors for the opposing client
	host_generate_anchor(client_id, CHAIN_SIGN, packet.sign_anchor);
	host_generate_anchor(client_id, CHAIN_ACK, packet.ack_anchor);

	challenge_response_t challenge_response;

	challenge_response.challenge = htonl(host_get_challenge(client_id));
	memcpy(challenge_response.sign_anchor, packet.sign_anchor, HASHSIZE);
	memcpy(challenge_response.ack_anchor, packet.ack_anchor, HASHSIZE);
	host_prepare_ack_packet(client_id, challenge_response.return_sign_anchor, challenge_response.return_ack_anchor);

	//host_print_challenge_reponse(&challenge_response);

	unsigned char challenge_response_sig[HASHSIZE];
	create_digest((unsigned char *) &challenge_response, sizeof(challenge_response), challenge_response_sig);
	#ifdef DEBUG_DIGEST
	char ackbuf[2*HASHSIZE+1];
	statusmsg("<%d> Digest of challenge_response is %.*s...\n", client_id, DIGEST_PRINTLEN, digeststr(challenge_response_sig, ackbuf));
	#endif

	host_sign_ack(conf, (unsigned char *) &challenge_response_sig, HASHSIZE, (unsigned char *) &(packet.challenge_response));

#ifdef DEBUG_HANDSHAKE
//[[
#ifdef DEBUG_DIGEST
	char signbuf[2*HASHSIZE+1], sigbuf[2*SIGSIZE+1];
#endif

	statusmsg("<%d> Sending ACK"
#ifdef DEBUG_DIGEST
		" --   SIGN: %.*s...,  ACK: %.*s..., SIG: %.*s..."
#endif
		"\n", client_id
#ifdef DEBUG_DIGEST
		, DIGEST_PRINTLEN, digeststr(packet.sign_anchor, signbuf),
		  DIGEST_PRINTLEN, digeststr(packet.ack_anchor, ackbuf),
		  DIGEST_PRINTLEN, digeststr(packet.challenge_response, sigbuf)
#endif
	);
#endif //]]

	return ap_protocol_send_udp(conf, client_id, &packet, sizeof(alpha_packet_ack_t));
} //]]

/** Send ACKACK handshake packet
 * @param	conf		Configuration-Pointer
 * @param	client_id	Client to send to
 */
socklen_t ap_protocol_send_ackack(const config_t *conf, const int client_id) {
//[[
	alpha_packet_ackack_t packet;
	memset(&packet, 0, sizeof(alpha_packet_ackack_t));
	packet.type = PACKET_ACKACK;

	// We're sending the ACKACK, so the first round of hash-anchors has successfully been used
	// Don't forget to decrement on receiving ACKACK as well!
	host_dec_hash_rounds(client_id, CHAIN_SIGN);
	host_dec_hash_rounds(client_id, CHAIN_ACK);
	host_generate_anchor(client_id, CHAIN_ACK, packet.ack);
	host_dec_hash_rounds(client_id, CHAIN_ACK);

#ifdef DEBUG_HANDSHAKE
	statusmsg("<%d> Sending ACKACK\n", client_id);
#endif

	return ap_protocol_send_udp(conf, client_id, &packet, sizeof(alpha_packet_ackack_t));
} //]]

/** Handle packets from client during HANDSHAKE state
 * @param	conf		Configuration-Pointer
 * @param	client_id	Sender of the packets
 * @param	packet		The packet
 * @param	bytes		Size of the packet (bytes)
 */
static void ap_protocol_handshake_state_machine(const config_t *conf, const unsigned int client_id, alpha_packet_t* packet, const unsigned int bytes) {
//[[

	assert(conf != NULL);
	assert(packet != NULL);
	assert(bytes > 0);

	/*   - ASS_STATE_SENT_CONNECT_WAIT_RETURN_CONNECT:
	 *     - RETURN_CONNECT: Send SYN with addr_hash
	 *     - * : ignore
	 *   - ASS_STATE_NEW:
	 *     - tun: buffer packet, send SYN and go into ASS_STATE_SENT_SYN_WAIT_ACK
	 *     - SYN: send ACK and go into ASS_STATE_SENT_ACK_WAIT_ACKACK
	 *     - * : ignore
	 *   - ASS_STATE_SENT_SYN_WAIT_ACK:
	 *     - tun: buffer packet
	 *     - SYN: TODO both parties started handshake
	 *     - ACK: send ACKACK and go into ASS_STATE_READY
	 *     - T.O: retransmit SYN packet
	 *     - * : ignore
	 *   - ASS_STATE_SENT_ACK_WAIT_ACKACK:
	 *     - tun: buffer packet
	 *     - SYN: retransmit ACK
	 *     - ACKACK: go into ASS_STATE_READY
	 *     - T.O: go into ASS_STATE_NEW
	 *     - * : ignore
	 *   - ASS_STATE_READY:
	 *     - tun: buffer packet
	 *     - SYN: send ACK and go into ASS_STATE_SENT_ACK_WAIT_ACKACK (peer died and started new handshake)
	 *     -  * : ignore
	 */

	// Handshake state machine
	switch(host_get_state(client_id, ASS_TRANS_MODE_HANDSHAKE)) {
	//[[
		// we received a reply to our CONNECT and are now returning the hash
		case ASS_STATE_SENT_CONNECT_WAIT_RETURN_CONNECT:
	 	//[[
			if(packet->type == PACKET_RETURN_CONNECT) {
#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got CONNECT_RETURN with addr_hash %.*s\n", client_id, DIGEST_PRINTLEN, digeststr(((alpha_packet_return_connect_t *)packet)->addr_hash, NULL));
#endif
				host_set_addr_hash(client_id, ((alpha_packet_return_connect_t*)packet)->addr_hash);
				ap_protocol_send_syn(conf, client_id);
				host_set_state(client_id, 0, ASS_STATE_SENT_SYN_WAIT_ACK);
			}
			break;
		//]]

		// Client was just created, no handshake done yet
		case ASS_STATE_NEW:
		//[[

			if(packet->type == PACKET_SYN) {

#ifdef DEBUG_HANDSHAKE
//[[
#ifdef DEBUG_DIGEST
				char signbuf[2*HASHSIZE+1], ackbuf[2*HASHSIZE+1];
#endif
				statusmsg("<%d> Got SYN"
#ifdef DEBUG_DIGEST
					"     --   SIGN: %.*s...,  ACK: %.*s..., CHALLENGE: %d"
#endif
					"\n", client_id
#ifdef DEBUG_DIGEST
					, DIGEST_PRINTLEN, digeststr(((alpha_packet_syn_t *) packet)->sign_anchor, signbuf),
					  DIGEST_PRINTLEN, digeststr(((alpha_packet_syn_t *) packet)-> ack_anchor,  ackbuf),
					ntohl(((alpha_packet_syn_t *) packet)->challenge)
#endif
				);
#endif //]]

				// if we're in pubkey-mode we have to generate the sigs
				if(!conf->private_key) {
					print_error("<%> Got SYN, but we don't have a private key. Aborting...", client_id);
					return;
				}

				// We received the anchors for this connection
				host_new_anchor(client_id, CHAIN_SIGN, ((const alpha_packet_syn_t *) packet)->sign_anchor);
				host_new_anchor(client_id, CHAIN_ACK,  ((const alpha_packet_syn_t *) packet)->ack_anchor);
				host_set_challenge(client_id, ntohl(((const alpha_packet_syn_t *) packet)->challenge));

				ap_protocol_send_ack(conf,client_id);
				host_set_timestamp(client_id, HANDSHAKE_TIMEOUT, 0);
				host_set_state(client_id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_SENT_ACK_WAIT_ACKACK);

			} else {

				// Initiate a handshake if the other side sends us stuff (this can for example
				// happen if our alpha gets restarted, and there was a successful handshake before)
#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got non-SYN packet, but handshake has not started. Sending SYN.\n", client_id);
#endif
				host_set_timestamp(client_id, HANDSHAKE_TIMEOUT, 0);
				ap_protocol_send_connect(conf, client_id);
				host_set_state(client_id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_SENT_SYN_WAIT_ACK);

			}

			break;
		//]]

		// We sent SYN and are waiting for ACK
		case ASS_STATE_SENT_SYN_WAIT_ACK:
		//[[
			if(packet->type == PACKET_ACK) {

#ifdef DEBUG_HANDSHAKE
//[[
#ifdef DEBUG_DIGEST
				char signbuf[2*HASHSIZE+1], ackbuf[2*HASHSIZE+1], sigbuf[2*SIGSIZE+1];
#endif
				statusmsg("<%d> Got ACK"
#ifdef DEBUG_DIGEST
					"     --   SIGN: %.*s...,  ACK: %.*s..., SIG: %.*s..."
#endif
					"\n", client_id
#ifdef DEBUG_DIGEST
					, DIGEST_PRINTLEN, digeststr(((alpha_packet_ack_t*)packet)->sign_anchor, signbuf),
					  DIGEST_PRINTLEN, digeststr(((alpha_packet_ack_t*)packet)->ack_anchor, ackbuf),
					  DIGEST_PRINTLEN, digeststr(((alpha_packet_ack_t*)packet)->challenge_response, sigbuf)
#endif
				);
#endif
//]]

				if(!host_verify_ack(client_id, (const alpha_packet_ack_t *) packet)) {
					statusmsg("<%d> Got ACK-packet with incorrect signature!\n", client_id);
					return;
				}

				// We received the anchors for this connection
				host_new_anchor(client_id, CHAIN_SIGN, ((const alpha_packet_ack_t *) packet)->sign_anchor);
				host_new_anchor(client_id, CHAIN_ACK, ((const alpha_packet_ack_t *) packet)->ack_anchor);
				ap_protocol_send_ackack(conf,client_id);

				// if we initiated the handshake and got ACK, we are done.
				host_set_state(client_id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_READY);
				host_set_state(client_id, ASS_TRANS_MODE_SENDING, ASS_STATE_READY);
				host_set_state(client_id, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);

				//this association is not "ready" because we do not want to transmit regular data on this channel
				//host_insert_ready_association(client_id, host_get_default_association(client_id));

				ca_request_new_asss(client_id, conf, conf->num_alpha_n, conf->num_alpha_c, conf->num_alpha_m, conf->num_alpha_z);

			} else if(packet->type == PACKET_SYN) {

				host_set_state(client_id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_NEW);
#ifdef DEBUG_HANDSHAKE
				print_error("<%d> Got SYN packet, but WE just sent a SYN packet. What to do??\n", client_id);
#endif

			} else {

				// Ignore all non-ACK packets while in state WAIT-ACK
#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got non-ACK packet while awaiting ACK. Ignoring.\n", client_id);
#endif

			}

			break;
		//]]

		// The other sclient_ide sent SYN, we sent ACK and wait for ACKACK
		case ASS_STATE_SENT_ACK_WAIT_ACKACK:
		//[[

			if(packet->type == PACKET_ACKACK) {


#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got ACKACK\n", client_id);
#endif

				// We received the ACKACK, so the first round of hash-anchors has successfully been used
				// Don't forget to decrement on sending ACKACK as well!
				host_dec_hash_rounds(client_id, CHAIN_SIGN);
				host_dec_hash_rounds(client_id, CHAIN_ACK);
				host_new_anchor(client_id, CHAIN_ACK, ((const alpha_packet_ackack_t *) packet)->ack);

				// if the other sclient_ide initiated the handshake and
				// we get our ACKACK, the handshake is done
				host_set_state(client_id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_READY);
				host_set_state(client_id, ASS_TRANS_MODE_SENDING, ASS_STATE_READY);
				host_set_state(client_id, ASS_TRANS_MODE_RECEIVING, ASS_STATE_READY);

				//this association is not "ready" because we do not want to transmit regular data on this channel
				//host_insert_ready_association(client_id, host_get_default_association(client_id));

				ca_request_new_asss(client_id, conf, conf->num_alpha_n, conf->num_alpha_c, conf->num_alpha_m, conf->num_alpha_z);

			} else if(packet->type == PACKET_SYN) {

#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got another SYN packet. Sending ACK (again).\n", client_id);
#endif

				ap_protocol_send_ack(conf,client_id);

			} else {
				// Ignore all non-ACKACK packets while awaiting ACKACK

#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got non-ACKACK packet while awaiting ACKACK. Ignoring.\n", client_id);
#endif
			}

			break;
		//]]

		// The handshake has already been completed before
		// TODO: Do we really want to accept handshakes here? New anchors can be distributed with control-packets
		case ASS_STATE_READY:
		//[[

			if(packet->type == PACKET_SYN) {

#ifdef DEBUG_HANDSHAKE
				statusmsg("<%d> Got a new SYN packet. %s has initiated a NEW handshake! Sending ACK.\n",
					client_id, inet_ntoa(host_get_addr(client_id)));
#endif
				// Discard our existing associations, as the opposing host doesn't know about them
				host_reset_associations(client_id, conf);
				// The opposing client started over, so it is using new hash anchors as well!
				// Good chance for us to generate and use new chains as well
				host_init_new_chains(client_id, conf);
				host_new_anchor(client_id, CHAIN_SIGN, ((const alpha_packet_syn_t *) packet)->sign_anchor);
				host_new_anchor(client_id, CHAIN_ACK, ((const alpha_packet_syn_t *) packet)->ack_anchor);
				host_set_challenge(client_id, ntohl(((const alpha_packet_syn_t *) packet)->challenge));

				ap_protocol_send_ack(conf,client_id);
				host_set_timestamp(client_id, HANDSHAKE_TIMEOUT, 0);
				host_set_state(client_id, ASS_TRANS_MODE_HANDSHAKE, ASS_STATE_SENT_ACK_WAIT_ACKACK);

			}

			// Other packets of that state are handled in the other switch()es

			break;
		//]]

	} //]]

} //]]

/** Handle packets from client during CONNECTION state
 * @param	conf		Configuration-Pointer
 * @param	client_id	Sender of the packets
 * @param	packet		The packet
 * @param	bytes		Size of the packet (bytes)
 */
static void ap_protocol_signature_state_machine(const config_t *conf, const unsigned int client_id, alpha_packet_t* packet, const unsigned int bytes) {
//[[

	assert(conf != NULL);
	assert(packet != NULL);
	assert(bytes > 0);

	bool valid_packet;
	unsigned char* payload_ptr = NULL;
	size_t payload_size = 0;

	// Signature scheme state machine
	//[[

	switch(packet->type) {

		case PACKET_S1:
		//[[

			switch(host_handle_s1_packet(client_id, conf, (const alpha_packet_s1_t*)packet, bytes - sizeof(alpha_packet_s1_t), &valid_packet)) {
				case AP_ERR_SUCCESS:
					// AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST,"<%d> Got S1\n", client_id);
					break;
				case AP_ERR_STATE:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S1, but we are not ready to receive!\n", client_id, packet->association_id);
					break;
				case AP_ERR_INVALID_MODE:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S1, but it has an unsupported Alpha mode.\n", client_id, packet->association_id);
					break;
				case AP_ERR_INVALID_ASS:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S1, but the requested association does not exists.\n", client_id, packet->association_id);
					break;
				default:
					AP_MSG_F(AP_MSG_LVL_WARN, AP_MSG_CTX_ST, "<%d, %3d> Handling of S1 packet failed due to unknown error!\n", client_id, packet->association_id);
			}

			if(!valid_packet)
				AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, ("<%d, %3d> Got S1, but it is not valid. Dropping it...\n"), client_id, packet->association_id);

			break;
		//]]

		case PACKET_A1:
		//[[

			switch(host_handle_a1_packet(client_id, conf, (const alpha_packet_a1_t*)packet, &valid_packet)) {
				case AP_ERR_SUCCESS:
					// AP_MSG_F(AP_MSG_LVL_VERBOSE, AP_MSG_CTX_ST,"<%d> Got A1\n", client_id);
					break;
				case AP_ERR_STATE:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got A1, but we are not ready to receive!\n", client_id, packet->association_id);
					break;
				case AP_ERR_INVALID_MODE:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got A1, but it has an not supported Alpha mode.\n", client_id, packet->association_id);
					break;
				case AP_ERR_INVALID_ASS:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got A1, but the requested association does not exists.\n", client_id, packet->association_id);
					ca_kill_ass(client_id, conf, (const unsigned int *) &packet->association_id, 1);
					break;
				default:
					AP_MSG_F(AP_MSG_LVL_WARN, AP_MSG_CTX_ST, "<%d, %3d> Handling of A1 packet failed due to unknown error!\n", client_id, packet->association_id);
			}

			if(!valid_packet)
				AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, ("<%d, %3d> Got A1, but it is not valid. Dropping it...\n"), client_id, packet->association_id);

			break;
		//]]

		case PACKET_ALPHA_Z:
		case PACKET_S2:
		//[[
			switch(host_handle_s2_packet(client_id, // client id
					conf, // config
					(alpha_packet_s2_t*)packet, // the actual packet
					(unsigned char*)packet + sizeof(alpha_packet_s2_t), // payload
					bytes - sizeof(alpha_packet_s2_t), // payload size
					&valid_packet,
					&payload_ptr,
					&payload_size)) {

				case AP_ERR_SUCCESS:
					break;
				case AP_ERR_STATE:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S2 , but we did not send A1. Something went wrong.\n", client_id, packet->association_id);
					break;
				case AP_ERR_INVALID_MODE:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S2 , but it has an not supported Alpha mode.\n", client_id, packet->association_id);
					break;
				case AP_ERR_INVALID_ASS:
					AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S2, but the requested association does not exists.\n", client_id, packet->association_id);
					break;
				default:
					AP_MSG_F(AP_MSG_LVL_WARN, AP_MSG_CTX_ST, "<%d, %3d> Handling of S2 packet failed due to unknown error!\n", client_id, packet->association_id);
			}

			if(!valid_packet) {
				AP_MSG_F(AP_MSG_LVL_DEFAULT, AP_MSG_CTX_ST, "<%d, %3d> Got S2, but it is not valid. Dropping it...\n", client_id, packet->association_id);
				break;
			}

			// if the packet is payload, hand it over to the tun device
			if(ca_is_ctrl_packet((const alpha_packet_s2_t*)packet)) {
				ca_hdl_ctrl_packet(client_id, conf, payload_ptr, payload_size);
				break;
			}

#ifdef DEBUG_TUN
			struct ip *ip_header = (struct ip*)((char*)packet + sizeof(alpha_packet_s2_t));
			char src[16],dest[16];
			struct protoent *protocol = getprotobynumber(ip_header->ip_p);
			strncpy(src,  inet_ntoa(ip_header->ip_src), 16);
			strncpy(dest, inet_ntoa(ip_header->ip_dst), 16);

			statusmsg("<%d> <--%s--  %s packet from %s to %s (%hu bytes, checksum %x)\n",
				client_id, "capture", protocol->p_name, src, dest, ntohs(ip_header->ip_len),
				ntohs(ip_header->ip_sum));
#endif

			// Write packet to tunnel
			if(write(conf->socket_outgoing, payload_ptr, payload_size) == -1) {
						print_error("write(): %s\n", strerror(errno));
			}

			break; //]]


	} //]]

} //]]

/** Handle packets from client and dispatch to HANDSHAKE or CONNECTION routines
 * @param	conf		Configuration-Pointer
 * @param	client_id	Sender of the packets
 * @param	packet		The packet
 * @param	bytes		Size of the packet (bytes)
 */
void ap_protocol_process_udp_packet(const config_t *conf, const int client_id, alpha_packet_t *packet, const int bytes) {
//[[
	assert(packet != NULL);
	assert(bytes > 0);

	if(host_get_state(client_id, ASS_TRANS_MODE_HANDSHAKE) != ASS_STATE_READY || packet->type == PACKET_SYN) {
		ap_protocol_handshake_state_machine(conf, client_id, packet, bytes);
	} else if(host_get_state(client_id, ASS_TRANS_MODE_HANDSHAKE) == ASS_STATE_READY) {
		ap_protocol_signature_state_machine(conf, client_id, packet, bytes);
	}

} //]]
