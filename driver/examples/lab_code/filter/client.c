// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>

#include "defines.h"
#include "client.h"
#include "association_relay.h"
#include "../digest.h"
#include "../control_association.h"
#include "../cache_tree.h"
#include "../extended_math.h"
#include "../xmalloc.h"
#include "../packet.h"

struct s_client_pair *relay_clients = NULL;
unsigned int clientslen = 0;

//! Selects the sender from a pair of clients
/*!
 * Checks if the first or the second client in the pair can be matched to the given sender address. 
 * 	
 * @param[in] client_pair_id the identification of the client pair
 * @param[in] sender the address of the sender
 * 
 * @note Do NOT use this function to check whether a sender is included in a pair, you will rise a assertion then 
 * @return either 0 or 1
 */
static unsigned int client_get_sender(const unsigned int client_pair_id, struct in_addr sender) {
//[[

	if(relay_clients[client_pair_id].c[0].addr.s_addr == sender.s_addr) {
		return 0;
	}

	if(relay_clients[client_pair_id].c[1].addr.s_addr == sender.s_addr) {
		return 1;
	}

	//this NEVER may happen (pairs only contain 2 elements NO more!)
	assert(1 == 0);
} //]]

//! Initializes a single client
/*!
 * Initializes a client. Each client should be contained in at least one client pair. 
 * @param[in] relay the already allocated relay that should be initialized
 * @param[in] addr the address of the client
 * @param[in] port the port of the client
 */
void client_init(struct alpha_client_relay* relay, const struct in_addr addr, const unsigned short port) {
//[[
	assert(relay != NULL);
	relay->addr = addr;
	relay->port = port;

	// create associations buffer
	relay->associations = xmalloc(sizeof(struct association_relay*) * NUM_ASSS);	
	memset(relay->associations, 0, sizeof(struct association_relay*) * NUM_ASSS);
} //]]

/** Free all heap memory */
void client_free() {
//[[

	struct s_client_pair *ptr = relay_clients;

	unsigned int i;
	unsigned int j;
	unsigned int k;
	for(i=0; i<clientslen; i++) {

		if(ptr == NULL) {
			continue;
		}

		printf("Freeing associations of client pair %d.\n", i);

		for(j = 0; j < 2; ++j) {
			for(k = 0; k < NUM_ASSS; ++k) {
				if(ptr->c[j].associations[k]) {
					association_relay_free(ptr->c[j].associations[k]);
				}
			}
		}
		xfree(ptr->c[0].associations);
		xfree(ptr->c[1].associations);
		ptr++;

	}

	puts("Freeing clients.");
	if(relay_clients) {
		xfree(relay_clients);
	}

} //]]

/** Add a client pair
 ** (Note that, in principal, it is possible to add a client pair twice! But this should not happen!)
 * @param	first	first client
 * @param	second	second client
 * @return	On success, the relay_clients id is returned (which will be the total number of relay_clients minus 1), otherwise -1.
 */
int client_add_pair(struct alpha_client_relay first, struct alpha_client_relay second) {
//[[

	assert(first.addr.s_addr != 0 && second.addr.s_addr != 0 && first.addr.s_addr != second.addr.s_addr);

	// Without loss of generality, the first argument is considered the smaller one
	// (this makes the other functions which access the relay_clients list a bit faster
	// and we have a "normalization", thus we save some checks later, because
	// we dont have the situation that there are two entries (a,b) and (b,a),
	// its always (a,b) if a<b)
	if(first.addr.s_addr > second.addr.s_addr) {
		return client_add_pair(second, first);
	}

	relay_clients = realloc(relay_clients, (clientslen+1) * sizeof(struct s_client_pair));
	if(relay_clients == NULL) {
		print_error("realloc() failed: %s\n", strerror(errno));
		return -1;
	}

	// The first one is smaller, we already know that by now
	relay_clients[clientslen].c[0] = first;
	relay_clients[clientslen].c[1] = second;

	return clientslen++;

} //]]

/** Find an association
 * This function will search the internal data structure for the association, identified by ass_id (in sender's list of associations)
 * @param	client_pair_id	to which client pair does the association belong?
 * @param	sender		to whom does the association belong?
 * @param	ass_id		association_id from the alpha packet
 * @return	On error, NULL is returned, otherwise, a pointer to the association
 */
struct association_relay* client_find_association(int client_pair_id, struct in_addr sender, unsigned int ass_id) {
//[[
	assert(ass_id < NUM_ASSS);
	return relay_clients[client_pair_id].c[client_get_sender(client_pair_id, sender)].associations[ass_id];
} //]]

/** Add an association to a client in a client pair (if it does not already exist)
 ** (Note that this will not work if sender is not one of the relay_clients in the client pair identified by client_pair_id)
 * @param	client_pair_id	id of the client pair
 * @param	sender		address of the sender of the packet (that is, the "owner" of the association)
 * @param	relay	the relay to be added
 */
void client_add_association(unsigned int client_pair_id, struct in_addr sender, struct association_relay* relay) {
//[[

	assert(client_pair_id <= clientslen);
	assert(relay != NULL);
	assert(relay->id < NUM_ASSS);

	// Find the alpha_client_relay with address "sender" in the selected client_pair (if its not the first, is has to be the second)
	struct alpha_client_relay *c = &(relay_clients[client_pair_id].c[ (relay_clients[client_pair_id].c[0].addr.s_addr == sender.s_addr) ? 0 : 1 ]);
	c->associations[relay->id] = relay;
	
} //]]

/** Find a client pair
 * @param	first		address of the first client
 * @param	first_port	port of the first client
 * @param	second		address of the second client
 * @param	second_port	port of the second client
 * @return	If the client exists, its id is returned, otherwise -1.
 */
int client_find_pair(struct in_addr first, unsigned short first_port, struct in_addr second, unsigned short second_port) {
//[[

	assert(first.s_addr != 0 && second.s_addr != 0 && first.s_addr != second.s_addr);

	if(first.s_addr > second.s_addr) {
		return client_find_pair(second, second_port, first, first_port);
	}

	unsigned int i;
	for(i=0; i<clientslen; i++) {

		if(relay_clients[i].c[0].addr.s_addr != first.s_addr) continue;
		if(relay_clients[i].c[0].port != first_port) continue;
		if(relay_clients[i].c[1].addr.s_addr != second.s_addr) continue;
		if(relay_clients[i].c[1].port != second_port) continue;

		// If we reach this line, we found a match
		return i;

	}

	return -1;

} //]]

/** Get the hmac of a client in a client pair
 ** (Note that this will not work properly if `addr' is not one of the relay_clients contained in the client pair identified by `id'!)
 * @param	id	the client pairs id
 * @param	ass_id	association id
 * @param	addr	the relay_clients addr
 * @return	a pointer to addr's last hmac in the pair id
 */
/*
 * unsigned char* client_get_hmac(struct association_relay *ass) {
//[[
	assert(ass != NULL);
	return ass->hmac;
} //]]
*/

//! Handles an incoming control packet
/*!
 * For a given control packet and a sender and client pair a function is 
 * executed. The function depends on the control packet type. 
 * 	
 * @remark Control packets are encapsulated in regular Alpha packets
 * @param[in] client_pair_id the id of the client pair in the clients array
 * @param[in] sender the address of the client that has sent the control packet
 * @param[in] packet pointer to the actual control packet (does not include the Alpha header, only the Alpha Control packet header)
 * @param[in] size size of the control packet
 */
void client_handle_control_packet(int client_pair_id, struct in_addr sender, struct alpha_control_packet *packet, const unsigned int size) {
//[[
	assert(packet != NULL);
	assert(size > 0);
	switch(packet->type) {
		case PACKET_NEW_ASS:
			association_relay_handle_new_associations(client_pair_id, sender, (struct alpha_packet_new_association*)packet, size);
			break;
		case PACKET_NEW_ASS_ACK:
			association_relay_handle_new_associations_ack(client_pair_id, sender, (struct alpha_packet_new_ass_ack*)packet, size);
			break;
		case PACKET_ASS_DIE:
			//is not handles yet
			// \TODO implement, but this is not really required to run Alpha
			//assert(1 == 0);
			break;
	}
} //]]
