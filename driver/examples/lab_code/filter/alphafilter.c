// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	alphafilter.c
 * @brief	ALPHA filter for intermediate routers, used to drop invalid alpha packets
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <libipq.h>
#include <linux/netfilter.h>

#include "defines.h"
#include "client.h"
#include "../packet.h"
#include "../digest.h"
#include "../control_association.h"
#include "../xmalloc.h"
#include "association_relay.h"
#include "association_relay_alpha_n.h"

// Maximum packet size, IPQ will try to handle
#define BUFSIZE 65535

#define REDON  "\033[31;;31m"
#define REDOFF "\033[0;;0m"

// ACCEPT a packet
#define PACKET_OKAY { if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, 0, NULL) < 0) { ipq_die(ipq_handle); }; continue; }

// calculate number of presignatures in a S1 packet
//#define NUMPRESIGS(udp_hdr, alphapacket) ( (udp_hdr->len - sizeof(struct udphdr) - sizeof(alpha_packet_s1_t)) / HASHSIZE )

// base ptr for n-th presignature in S1 packet (start counting by 0, not 1)

// This should be global in order to free() it in the signal handler. No biggie.
struct ipq_handle *ipq_handle;

static void ipq_die(struct ipq_handle *h) {
//[[
	ipq_perror(NULL);
	ipq_destroy_handle(h);
	exit(EXIT_FAILURE);
} //]]

char* type_to_str(unsigned char type) {
//[[
	static char ret[10];

	switch(type) {

		case PACKET_SYN:		strcpy(ret, "SYN");	break;
		case PACKET_ACK:		strcpy(ret, "ACK");	break;
		case PACKET_ACKACK:		strcpy(ret, "ACKACK");	break;
		case PACKET_S1:			strcpy(ret, "S1");	break;
		case PACKET_A1:			strcpy(ret, "A1");	break;
		case PACKET_S2:			strcpy(ret, "S2");	break;
		case PACKET_CONNECT:		strcpy(ret, "CON");	break;
		case PACKET_RETURN_CONNECT:	strcpy(ret, "CONRET");	break;
		case PACKET_NEW_ASS:		strcpy(ret, "NA");	break;
		case PACKET_NEW_ASS_ACK:	strcpy(ret, "NAACK");	break;

		default:
			strcpy(ret, "unknown");
			break;

	}

	return ret;
} //]]

void sigterm(int sig) {
//[[
	printf("Received SIG%s. Exiting.\n", (sig == 15) ? "TERM" : "INT");

	ipq_destroy_handle(ipq_handle);

	// Free memory
	client_free();

	exit(EXIT_FAILURE);
} //]]

int main(void) {
//[[

	if(getuid() != 0) {
		printf("You should probably be root to do this.\n");
	}

	// Create libipq handle
	ipq_handle = ipq_create_handle(0, PF_INET);
	if(ipq_handle == NULL) {
		ipq_die(ipq_handle);
	}

	// Copy packet metadata and payload to userspace
	if(ipq_set_mode(ipq_handle, IPQ_COPY_PACKET, BUFSIZE) == -1) {
		ipq_die(ipq_handle);
	}

	// Set SIGTERM and SIGINT handler
	signal(SIGTERM, sigterm);
	signal(SIGINT,  sigterm);

	// now we tune the libipq sockets to receive more data
	unsigned int bufsize = 80000;
	setsockopt(ipq_handle->fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(unsigned int));

	while(1) {

		unsigned char buf[BUFSIZE];

		// Read data from kernel to buffer (and block, if there is no data available)
		if(ipq_read(ipq_handle, buf, BUFSIZE, 0) == -1) {
			//ipq_die(ipq_handle);
			print_error("Error Buffer: %s\n", ipq_errstr());
			continue;
		}

		// Ignore this packet if an error occured
		if(ipq_message_type(buf) == NLMSG_ERROR) {
			print_error("Received error message %d (%s)\n", ipq_get_msgerr(buf), strerror(ipq_get_msgerr(buf)));
			continue;
		}

		// Get the content of the packet we just received
		ipq_packet_msg_t *m = ipq_get_packet(buf);

		// IP header of packet
		struct ip *ip_hdr = (struct ip*)m->payload;

		// We are only interested in UDP packets
		if(ip_hdr->ip_p != IPPROTO_UDP) {
			PACKET_OKAY;
		}

		// UDP header of packet (the udp header is exactly ip_hdr->ip_hl 32 bit words
		// behind the first byte of the IP header)
		struct udphdr *udp_hdr = (struct udphdr*)((uint32_t*)ip_hdr + ip_hdr->ip_hl);

		// We are only interested in UDP packets which are addressed to the ALPHA port
		if(ntohs(udp_hdr->dest) != PORT) {
			PACKET_OKAY;
		}

		// Alpha packet header
		alpha_packet_t *packet = (alpha_packet_t*)((char*)udp_hdr + sizeof(struct udphdr));

		// Ignore packets that can not possibly be alpha packets, because the packet type is not valid
		if(packet->type < PACKET_LOWEST || packet->type > PACKET_HIGHEST) {
			PACKET_OKAY;
		}

		// Ignore alpha z packets
		if(packet->type == PACKET_ALPHA_Z) {
			PACKET_OKAY;
		}

		// Ignore packets which can not possibly be alpha packets, because they are not even as long as the
		// smallest alpha packet possible (alpha_packet_t)
		if(ntohs(udp_hdr->len) - sizeof(struct udphdr) < sizeof(alpha_packet_t)) {
			PACKET_OKAY;
		}

#ifdef DEBUG
		// Hosts as strings for later debugging use
		char srchost[16], dsthost[16];
		strcpy(srchost, inet_ntoa(ip_hdr->ip_src));
		strcpy(dsthost, inet_ntoa(ip_hdr->ip_dst));
#endif

		// Check if we already now this communication (aka this client pair), if not, add it
		int id = client_find_pair(ip_hdr->ip_src, ntohs(udp_hdr->source), ip_hdr->ip_dst, ntohs(udp_hdr->dest));
		if(id == -1) {

			struct alpha_client_relay first, second;

			// Zero all fields (especially the pointer to the associations)
			memset(&first, 0, sizeof(struct alpha_client_relay));
			memset(&second, 0, sizeof(struct alpha_client_relay));

			client_init(&first, ip_hdr->ip_src, ntohs(udp_hdr->source));
			client_init(&second, ip_hdr->ip_dst, ntohs(udp_hdr->dest)); 

			id = client_add_pair(first, second);

		}


#ifdef DEBUG
//[[
		printf("<%d,%3d> %6s ", id, packet->association_id, type_to_str(packet->type));

		if(ip_hdr->ip_src.s_addr <= ip_hdr->ip_dst.s_addr) {
			printf("(%s -> %s)\t", srchost, dsthost);
		} else {
			printf("(%s <- %s)\t", dsthost, srchost);
		}
#endif //]]

		// Flag, indicating that the packet should be dropped at the end
		int drop = 0;
		struct association_relay *ass = NULL;
		bool valid;
		const unsigned int packet_len = ntohs(udp_hdr->len) - sizeof(struct udphdr) ;

		////////////////////////////////////////////////////////
		//                    FILTER ENGINE                   //
		//                                                    //
		//   Here we decide what to do with packets we see    //
		////////////////////////////////////////////////////////

		switch(packet->type) {

			case PACKET_SYN:
			//[[
				{
				// Add the default association_relay (always ALPHA_N) for the sender (if it does not already exist)
				// (the corresponding association_relay of the receiver will be added when
				// he sends ACK for this handshake)
				struct alpha_n_ass_relay* def_ass = xmalloc(sizeof(struct alpha_n_ass_relay));				
				association_relay_init_alpha_n_ass(def_ass, id, ALPHA_N);
				association_relay_set_sign_anchor((struct association_relay*)def_ass, ((alpha_packet_syn_t*) packet)->sign_anchor);
				association_relay_set_ack_anchor((struct association_relay*)def_ass, ((alpha_packet_syn_t*) packet)->ack_anchor);

				client_add_association(id, ip_hdr->ip_src, (struct association_relay*)def_ass);

#ifdef DEBUG
//[[
				printf(" -- SIGN: %.*s...", DIGEST_PRINTLEN, digeststr(((alpha_packet_syn_t*)packet)->sign_anchor, NULL));
				printf(   ", ACK: %.*s...", DIGEST_PRINTLEN, digeststr(((alpha_packet_syn_t*)packet)-> ack_anchor, NULL));
//]]
#endif
				break;
				}
			//]]

			case PACKET_ACK:
			//[[
				{
				// Add the default association (always ALPHA_N) (see PACKET_SYN case)
				struct alpha_n_ass_relay* def_ass = xmalloc(sizeof(struct alpha_n_ass_relay));				
				association_relay_init_alpha_n_ass(def_ass, id, ALPHA_N);
				association_relay_set_sign_anchor((struct association_relay*)def_ass, ((alpha_packet_ack_t*) packet)->sign_anchor);
				association_relay_set_ack_anchor((struct association_relay*)def_ass, ((alpha_packet_ack_t*) packet)->ack_anchor);

				client_add_association(id, ip_hdr->ip_src, (struct association_relay*)def_ass);

#ifdef DEBUG
//[[
				printf(" -- SIGN: %.*s...", DIGEST_PRINTLEN, digeststr(((alpha_packet_ack_t*)packet)->sign_anchor, NULL));
				printf(   ", ACK: %.*s...", DIGEST_PRINTLEN, digeststr(((alpha_packet_ack_t*)packet)-> ack_anchor, NULL));
//]]
#endif

				break;
				}
			//]]

			case PACKET_ACKACK:
			//[[

				ass = client_find_association(id, ip_hdr->ip_src, packet->association_id);

				// If we do not know this association, then we did not see the handshake or did not see
				// the bootstrapping.. Nothing we can do. ACCEPT this packet.
				if(ass == NULL) {
					drop = 1;
					break;
				}

#ifdef DEBUG
//[[
				{
				char output_anchor[HASHSIZE*2+1], output_digest[HASHSIZE*2+1], output_oldanchor[HASHSIZE*2+1];
				unsigned char digest[HASHSIZE];

				printf(" -- anchor %.*s..., hash %.*s..., expected %.*s...",
					DIGEST_PRINTLEN, digeststr(((alpha_packet_ackack_t*)packet)->ack, output_anchor),
					DIGEST_PRINTLEN, digeststr(create_digest(((alpha_packet_ackack_t*)packet)->ack, HASHSIZE, digest), output_digest),
					DIGEST_PRINTLEN, digeststr(association_relay_get_ack_anchor(ass), output_oldanchor));

				}
//]]
#endif

				if(association_relay_verify_ack_anchor(ass, ((alpha_packet_ackack_t*) packet)->ack)) {
#ifdef DEBUG
//[[
					printf(" <--- %sINVALID!%s Dropping.", REDON, REDOFF);
//]]
#endif
					drop=1;
					break;
				}

				association_relay_set_ack_anchor(ass, ((alpha_packet_ackack_t*)packet)->ack);
				break;

			//]]

			case PACKET_S1:
			//[[

				ass = client_find_association(id, ip_hdr->ip_src, packet->association_id);

				// If we do not know this association, then we did not see the handshake or did not see
				// the bootstrapping.. Nothing we can do. ACCEPT this packet.
				if(ass == NULL) {
					drop = 1;
					break;
				}

				int s1_invalid = association_relay_verify_sign_anchor(ass, ((alpha_packet_s1_t *) packet)->anchor);
#ifdef DEBUG
//[[
				{
				char output_anchor[HASHSIZE*2+1], output_digest[HASHSIZE*2+1], output_oldanchor[HASHSIZE*2+1];
				unsigned char digest[HASHSIZE];

				printf(" -- anchor %.*s..., hash %.*s..., expected %.*s..., HMAC: %.*s...",
					DIGEST_PRINTLEN, digeststr(((alpha_packet_s1_t *) packet)->anchor, output_anchor),
					DIGEST_PRINTLEN, digeststr(create_digest(((alpha_packet_s1_t *) packet)->anchor, HASHSIZE, digest), output_digest),
					DIGEST_PRINTLEN, digeststr(association_relay_get_sign_anchor(ass), output_oldanchor),
					DIGEST_PRINTLEN, digeststr((unsigned char*)packet + sizeof(alpha_packet_s1_t), NULL)
				);
				}
//]]
#endif

				if(s1_invalid) {
#ifdef DEBUG
//[[
					printf(" <--- %sINVALID!%s Dropping.", REDON, REDOFF);
//]]
#endif
					drop=1;
					break;
				}

				association_relay_set_sign_anchor(ass, ((alpha_packet_s1_t *) packet)->anchor);
				association_relay_handle_s1(ass, (alpha_packet_s1_t*)packet, packet_len);

				break;
			//]]

			case PACKET_A1:
			//[[

				ass = client_find_association(id, ip_hdr->ip_src, packet->association_id);

				// If we do not know this association, then we did not see the handshake or did not see
				// the bootstrapping.. Nothing we can do. ACCEPT this packet.
				if(ass == NULL) {
					drop = 1;
					break;
				}

				int a1_invalid = association_relay_verify_ack_anchor(ass, ((alpha_packet_a1_t *) packet)->anchor);
#ifdef DEBUG
//[[
				{
				char output_anchor[HASHSIZE*2+1], output_digest[HASHSIZE*2+1], output_oldanchor[HASHSIZE*2+1];
				unsigned char digest[HASHSIZE];

				printf(" -- anchor %.*s..., hash %.*s..., expected %.*s...",
					DIGEST_PRINTLEN, digeststr(((alpha_packet_a1_t *) packet)->anchor, output_anchor),
					DIGEST_PRINTLEN, digeststr(create_digest(((alpha_packet_a1_t *) packet)->anchor, HASHSIZE, digest), output_digest),
					DIGEST_PRINTLEN, digeststr(association_relay_get_ack_anchor(ass), output_oldanchor));

				}
//]]
#endif

				if(a1_invalid) {
					//! \TODO Verify return anchor as well!
#ifdef DEBUG
//[[
					printf(" <--- %sINVALID!%s Dropping.", REDON, REDOFF);
//]]
#endif
					drop=1;
					break;
				}

				association_relay_set_ack_anchor(ass, ((alpha_packet_a1_t *) packet)->anchor);
				break;
			//]]

			case PACKET_S2:
			//[[

				ass = client_find_association(id, ip_hdr->ip_src, packet->association_id);

				// If we do not know this association, then we did not see the handshake or did not see
				// the bootstrapping.. Nothing we can do. ACCEPT this packet.
				if(ass == NULL) {
					drop = 1;
					break;
				}


				int s2_invalid = association_relay_verify_sign_anchor(ass, ((alpha_packet_s2_t *) packet)->anchor);
#ifdef DEBUG
//[[
				{
				char output_anchor[HASHSIZE*2+1], output_digest[HASHSIZE*2+1], output_oldanchor[HASHSIZE*2+1];
				unsigned char digest[HASHSIZE];

				printf(" -- anchor %.*s..., hash %.*s..., expected %.*s...",
					DIGEST_PRINTLEN, digeststr(((alpha_packet_s2_t *) packet)->anchor, output_anchor),
					DIGEST_PRINTLEN, digeststr(create_digest(((alpha_packet_s2_t *) packet)->anchor, HASHSIZE, digest), output_digest),
					DIGEST_PRINTLEN, digeststr(association_relay_get_sign_anchor(ass), output_oldanchor)
				);

				}
//]]
#endif

				if(s2_invalid) {
#ifdef DEBUG
//[[
					printf(" <--- %sINVALID!%s Dropping.", REDON, REDOFF);
//]]
#endif
					drop=1;
					break;
				}

				association_relay_verify(ass, ((alpha_packet_s2_t *) packet)->anchor,
					((unsigned char*)packet),
					ntohs(udp_hdr->len) - sizeof(struct udphdr), &valid);

				if(!valid) {
#ifdef DEBUG
//[[
					printf(" <--- %sHMAC MISMATCH%s. Dropping.", REDON, REDOFF);
//]]
#endif
					drop=1;
					break;
				}



				if(ca_is_ctrl_packet((alpha_packet_s2_t*)packet)) {
					client_handle_control_packet(id, ip_hdr->ip_src, 
						(alpha_control_packet_t*)((unsigned char*)packet + sizeof(alpha_packet_s2_t)), 
						packet_len - sizeof(alpha_packet_s2_t));
				}

				break;
			//]]
		}

#ifdef DEBUG
//[[
		puts("");
//]]
#endif

		// ACCEPT the packet
		//if(ipq_set_verdict(ipq_handle, m->packet_id, (drop==0) ? NF_ACCEPT : NF_DROP, 0, NULL) < 0) {
		// FIXME: Temporary workaround so the filter won't drop anything!
		if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, 0, NULL) < 0) {
			ipq_die(ipq_handle);
		}

	}

	// Will never be executed
	ipq_destroy_handle(ipq_handle);

	return EXIT_SUCCESS;
} //]]
