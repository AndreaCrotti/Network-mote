// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __HOST_H_
#define __HOST_H_

#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>

#include "list.h"
#include "hashchain.h"
#include "packet.h"
#include "association.h"
#include "protocol.h"

/** @brief: Client data structure */
typedef struct alpha_client {
//[[
	//! The address of the client
	struct in_addr addr;
	//! check if this packet has already passed the TUN/TAP device
	bool delete_me;
	struct timeval timestamp;
	uint32_t id;

	char* public_key_file;
	DSA* public_key;
	uint32_t challenge;		// or "nonce", for handshake
	unsigned char addr_hash[HASHSIZE];	// the address-hash we're supposed to return

	//! Pointer to the default association
	/*!
	 * The default association is used for control messages. This
	 * association is the only one (per host) which has (or should have)
	 * the attribute "ASS_DIRECTION_BIDIRECTIONAL" in the
	 * direction property.
	 */
	struct association *default_association;
	//! Buffer containing associaions for sending data
	/*!
	 * 	Associations in this buffer can ONLY be used to send data.
	 */
	struct association **associations_out;
	//! Buffer containing associations for receiving data
	/*!
	 * 	Associations in this buffer can ONLY be used to receive data.
	 */
	struct association **associations_in;

	//! Ringbuffer containing free associations
	/*!
	 * 	Free means here that the associations can send data and
	 *  are in the ready state
	 */
	struct ring_buffer* free_associations;

	//! Pointer moving along free associations
	struct ring_buffer_iterator* free_associations_it;

	//! Temporary queue for packets which are later distributed over the associations
	list_t* packet_queue;

	//! the last id which was used to open a new association
	size_t last_ass_id;

	//! Pointer to a new default association if the current default association runs out of hash chain elements
	struct alpha_n_ass* new_def_ass;

	//! Timeout for the packet queue (i.e. when packets need to be flushed)
	struct timeval packet_timeout;

	//! Minimum timeout regarding ALL timeouts
	struct timeval min_timeout;

} alpha_client_t; //]]

/** @brief: Client list data structure */
struct s_clients {
//[[
	unsigned int len;
	struct alpha_client *c;
}; //]]

int host_init(const unsigned int client_id, const config_t*);

void host_free(const unsigned int client_id);

ap_err_code host_reset_associations(const uint32_t, const config_t*);

ap_err_code host_remove_association(const unsigned int, const unsigned int, const unsigned int);

struct alpha_n_ass* host_get_default_association(const uint32_t) ;

int host_find(const struct in_addr addr);

int host_add(config_t *conf, const char *name, const struct in_addr addr, char* public_key_file);

int host_read_from_file(config_t *conf);

void host_free_all(void);

DSA* host_get_pubkey(const unsigned int client_id);

int host_get_state(const unsigned int client_id, const int mode);

void host_set_state(const unsigned int client_id, const int mode, const int state);

struct in_addr host_get_addr(const unsigned int client_id);

ap_err_code host_enqueue_control_packet(const unsigned int client_id, unsigned char *data, const unsigned int datalen);

int host_enqueue_packet(const unsigned int client_id, unsigned char *data, const unsigned int datalen);

unsigned int host_get_queue_size(const unsigned int client_id);

void host_get_timestamp(const unsigned int client_id, struct timeval* timeout);

void host_set_timestamp(const unsigned int client_id, const unsigned int sec, const unsigned int usec);

unsigned int hosts_len(void);

ap_err_code host_distribute_packets(const unsigned int);

socklen_t host_process_packets_default_assocation(const config_t*, const unsigned int);

socklen_t host_process_packets(const config_t*, const unsigned int);

ap_err_code host_handle_timeouts(const unsigned int, const config_t*);

ap_err_code host_insert_ready_association(const unsigned int, struct association*);

ap_err_code host_handle_s1_packet(const unsigned int, const config_t*, const alpha_packet_s1_t*, const size_t, bool*);

ap_err_code host_handle_s2_packet(const unsigned int, const config_t*, const alpha_packet_s2_t*, unsigned char*, const size_t,
	bool* const valid, unsigned char**, size_t* const);

ap_err_code host_handle_a1_packet(const unsigned int, const config_t*, const alpha_packet_a1_t*, bool*);

size_t host_get_new_ass_id(const unsigned int);

ap_err_code host_add_association(const uint32_t, struct association*);

struct association* host_get_incoming_association(const unsigned int, const unsigned int);

struct association* host_get_outgoing_association(const unsigned int, const unsigned int);

struct alpha_n_ass* host_create_new_default_association(const unsigned int, const config_t*);

struct alpha_n_ass* host_get_new_default_association(const unsigned int);

ap_err_code host_update_default_association(const unsigned int);

/************************* CLIENT HASH FUNCTIONS ****************************/

void host_new_anchor(const unsigned int client_id, const int chain, const unsigned char *anchor);

unsigned char* host_generate_anchor(const unsigned int client_id, const int chain, unsigned char *digest);

unsigned char* host_sign_ack(const config_t* conf, unsigned char* message, size_t message_size, unsigned char* signature);

bool host_verify_sig(const unsigned int client_id, const unsigned char* message, const size_t message_size, unsigned char* signature);
bool host_verify_ack(const unsigned int client_id, const alpha_packet_ack_t *packet);

bool host_verify_anchor(const unsigned int client_id, const unsigned char *anchor, const int chain);

void host_dec_hash_rounds(const unsigned int client_id, const int chain);

int host_init_new_chains(const unsigned int, const config_t*);

uint32_t host_get_challenge(const unsigned int client_id);
void host_set_challenge(const unsigned int client_id, uint32_t challenge);

unsigned char* host_get_addr_hash(const unsigned int client_id);
void host_set_addr_hash(const unsigned int client_id, unsigned char *addr_hash);

void host_prepare_ack_packet(const unsigned int client_id, unsigned char *return_sign_anchor, unsigned char *return_ack_anchor);

void host_print_challenge_reponse(const challenge_response_t *cr);

void host_set_packet_timeout(const unsigned int, const unsigned int, const unsigned int);

void host_check_and_set_min_timeout(const unsigned int, const unsigned int, const unsigned int);

bool host_check_min_timeout_exceed(const unsigned int);

#define CHAIN_SIGN	1
#define CHAIN_ACK	2

#endif // __HOST_H_
