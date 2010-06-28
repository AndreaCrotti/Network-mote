// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	evil.c
 * @brief	ALPHA evil filter. This filter simulates a noisy/hostile environment, by randomly dropping or altering packets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

//#include <libipq.h>
#include <libipq.h>
#include <linux/netfilter.h>

#define HASHSIZE 20
#include "../packet.h"

#define EVIL_PORT "1234"
#define BUFSIZE 65535

// Define how frequent packet should be dropped and changed
// (undefine to disable)
//#define DROP 20
#define CHANGE 5
#define DONTCHANGES2
#define DONTDROPS2
#define ALWAYSFIRSTPART

// Care about incoming and outgoing packets?
// (undefine to disable)
#define INPUT
#define OUTPUT

/** die with error message
 * @param	h	the ipq_handle, on which the error occured
 */
static void ipq_die(struct ipq_handle *h) {
//[[
	ipq_perror(NULL);
	ipq_destroy_handle(h);
	exit(EXIT_FAILURE);
} //]]

/** Convert a packet type (reprensented by a number) to a string (used for logging)
 * @param	type	the packet type, conforming to packet.h
 * @return	The packet type's string representation is returned or "unknown" if type is not a valid packet type
 */
char* type_to_str(unsigned char type) {
//[[
	static char ret[10];

	switch(type) {
		case PACKET_SYN:    strcpy(ret, "SYN");     break;
		case PACKET_ACK:    strcpy(ret, "ACK");     break;
		case PACKET_ACKACK: strcpy(ret, "ACKACK");  break;
		case PACKET_S1:	    strcpy(ret, "S1");      break;
		case PACKET_A1:     strcpy(ret, "A1");      break;
		case PACKET_S2:     strcpy(ret, "S2");      break;
		default:            strcpy(ret, "unknown"); break;
	}

	return ret;
} //]]

/** Exit handler, gets called when the program terminates with exit() (or when reaching the end of main())
 ** used for deleting iptables rules
 */
static void exithandler(void) {
//[[
	puts("");
	printf("Deleting iptables QUEUE rules for udp:"EVIL_PORT"\n");
#ifdef OUTPUT
	if(system("iptables -D OUTPUT -p udp --dport "EVIL_PORT" -j QUEUE") == -1) {
		printf("Failed to delete iptables output QUEUE rule for udp:"EVIL_PORT"\n");
	}
#endif
#ifdef INPUT
	if(system("iptables -D INPUT  -p udp --dport "EVIL_PORT" -j QUEUE") == -1) {
		printf("Failed to delete iptables input QUEUE rule for udp:"EVIL_PORT"\n");
	}
#endif
	puts("");
} //]]

/** Signal handler for SIGINT, used to exit() after ^C, so that exithandler() gets called (and iptables rules deleted)
 * @param	sig	the signal (SIGINT)
 */
static void sigint(int sig) {
//[[
	exit(EXIT_SUCCESS);
} //]]

int main(void) {
//[[

	if(getuid() != 0) {
		printf("You should probably be root to do this.\n");
	}

	// Set exit handler and signal handler
	atexit(exithandler);
	signal(SIGINT, sigint);

	// Add iptables rules
	puts("");
	printf("Adding iptables QUEUE rules for udp:"EVIL_PORT"\n");
#ifdef OUTPUT
	if(system("iptables -A OUTPUT -p udp --dport "EVIL_PORT" -j QUEUE") == -1) {
		printf("Failed to add iptables output QUEUE rule for udp:"EVIL_PORT"\n");
	}
#endif
#ifdef INPUT
	if(system("iptables -A INPUT  -p udp --dport "EVIL_PORT" -j QUEUE") == -1) {
		printf("Failed to add iptables input QUEUE rule for udp:"EVIL_PORT"\n");
	}
#endif
	puts("");

	// Create libipq handle
	struct ipq_handle *ipq_handle = ipq_create_handle(0, PF_INET);
	if(ipq_handle == NULL) {
		ipq_die(ipq_handle);
	}

	// Copy packet metadata and payload to userspace
	if(ipq_set_mode(ipq_handle, IPQ_COPY_PACKET, BUFSIZE) == -1) {
		ipq_die(ipq_handle);
	}

	// Seed the pseudo random number generator
	srand(time(0) * getpid());

	while(1) {

		unsigned char buf[BUFSIZE];

		// Read data from kernel to buffer (and block, if there is no data available)
		if(ipq_read(ipq_handle, buf, BUFSIZE, 0) == -1) {
			ipq_die(ipq_handle);
		}

		// Ignore this packet if an error occured
		if(ipq_message_type(buf) == NLMSG_ERROR) {
			fprintf(stderr, "Received error message %d (%s)\n", ipq_get_msgerr(buf), strerror(ipq_get_msgerr(buf)));
			continue;
		}

		// Get the content of the packet we just received
		ipq_packet_msg_t *m = ipq_get_packet(buf);

		// IP header of packet
		struct ip *ip_hdr = (struct ip*)m->payload;
		assert(ip_hdr->ip_p == IPPROTO_UDP);

		// Hosts as string for later use
		char srchost[16], dsthost[16];
		strcpy(srchost, inet_ntoa(ip_hdr->ip_src));
		strcpy(dsthost, inet_ntoa(ip_hdr->ip_dst));

		// UDP header of packet (the udp header is exactly ip_hdr->ip_hl 32 bit words
		// behind the first byte of the IP header)
		struct udphdr *udp_hdr = (struct udphdr*)((uint32_t*)ip_hdr + ip_hdr->ip_hl);

		// Alpha packet header
		alpha_packet_t *packet = (alpha_packet_t*)((char*)udp_hdr + sizeof(struct udphdr));

		// Ignore packets that can not possibly be alpha packets
		if(packet->type < PACKET_LOWEST || packet->type > PACKET_HIGHEST) {
			if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, 0, NULL) < 0) {
				ipq_die(ipq_handle);
			}
		}

		printf("%6s packet from %s:%u to %s:%u (%3u bytes payload)", type_to_str(packet->type), srchost, ntohs(udp_hdr->source),
			dsthost, ntohs(udp_hdr->dest), ntohs(udp_hdr->len) - (unsigned short)sizeof(struct udphdr));

#ifdef DROP
		// randomly drop approximately one in DROP packets
		if(rand() % DROP == 0
#ifdef DONTDROPS2
			&& packet->type != PACKET_S2
#endif
) {
			printf(" <-- DROPPED! (%s)\n", type_to_str(packet->type));
			if(ipq_set_verdict(ipq_handle, m->packet_id, NF_DROP, 0, NULL) < 0) {
				ipq_die(ipq_handle);
			}
			continue;
		}
#endif

#ifdef CHANGE
		// Dont change SYN, ACK or ACKACK
		if(packet->type == PACKET_SYN || packet->type == PACKET_ACK || packet->type == PACKET_ACKACK
#ifdef DONTCHANGES2
			|| packet->type == PACKET_S2
#endif
) {
			puts("");
			if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, 0, NULL) < 0) {
				ipq_die(ipq_handle);
			}
			continue;
		}

		// Flip some bits in some random byte of the crypto part of the header
		if(rand() % CHANGE == 0) {

#ifdef ALWAYSFIRSTPART
			int foo = 0;
#else
			// Change first or second crypto field (always change first if there is no second)
			int foo = (packet->type == PACKET_S2) ? 0 : rand()%2;
#endif

			printf(" <-- CHANGED! (%s, %d)\n", type_to_str(packet->type), foo);

			// Randomly fill one of the crypto fields (i.e. THE crypto field, if there is only one) with garbage
			memset(((char*)packet + 1 + (foo * HASHSIZE)), (rand()%2 * 0xFF), HASHSIZE);

			// We changed the content of the UDP packet, therefore we need to recalculate the udp checksum
			// thats too complicated, just disable checksumming for this packet
			udp_hdr->check = 0;

			// ACCEPT the (modified) packet
			if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, m->data_len, m->payload) < 0) {
				ipq_die(ipq_handle);
			}

			continue;

		}
#endif

		puts("");

		// ACCEPT the packet
		if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, 0, NULL) < 0) {
			ipq_die(ipq_handle);
		}

	}

	// Will never be executed
	ipq_destroy_handle(ipq_handle);

	return EXIT_SUCCESS;
} //]]
