// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	filter_skeleton.c
 * @brief	Skeleton for an ipqueue filter, this does nothing.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include <libipq.h>
#include <linux/netfilter.h>

#define BUFSIZE 65535
#define DEBUG

/** Die with error message because of an ipqueue error
 * @param	h	the ipq handle on which the error occured
 */
static void ipq_die(struct ipq_handle *h) {
//[[
	ipq_perror(NULL);
	ipq_destroy_handle(h);
	exit(EXIT_FAILURE);
} //]]

int main(void) {
//[[

	if(getuid() != 0) {
		printf("You should probably be root to do this.\n");
	}

	// Create libipq handle
	struct ipq_handle *ipq_handle = ipq_create_handle(0, PF_INET);
	if(ipq_handle == NULL) {
		ipq_die(ipq_handle);
	}

	// Copy packet metadata and payload to userspace
	if(ipq_set_mode(ipq_handle, IPQ_COPY_PACKET, BUFSIZE) == -1) {
		ipq_die(ipq_handle);
	}

	while(1) {

		unsigned char buf[BUFSIZE];

		// Read data from kernel to buffer (and block, if there is no data available)
		if(ipq_read(ipq_handle, buf, BUFSIZE, 0) == -1) {
			ipq_die(ipq_handle);
		}

		// Ignore this packet if an error occured
		if(ipq_message_type(buf) == NLMSG_ERROR) {
			fprintf(stderr, "Received error message %d\n", ipq_get_msgerr(buf));
			continue;
		}

		// Get the content of the packet we just received
		ipq_packet_msg_t *m = ipq_get_packet(buf);

		// IP header of packet
		struct ip *ip_hdr = (struct ip*)m->payload;

		// Only UDP packets
		assert(ip_hdr->ip_p == IPPROTO_UDP);

		// UDP header of packet (the udp header is exactly ip_hdr->ip_hl 32 bit words
		// behind the first byte of the IP header)
		struct udphdr *udp_hdr = (struct udphdr*)((uint32_t*)ip_hdr + ip_hdr->ip_hl);

#ifdef DEBUG
		char srchost[16], dsthost[16];
		strcpy(srchost, inet_ntoa(ip_hdr->ip_src));
		strcpy(dsthost, inet_ntoa(ip_hdr->ip_dst));
		printf("-- UDP packet from %s:%u to %s:%u (%u bytes payload)\n", srchost, ntohs(udp_hdr->source),
			dsthost, ntohs(udp_hdr->dest), ntohs(udp_hdr->len) - sizeof(struct udphdr));
#endif


		//
		// Do something with the packet
		//


		// ACCEPT the packet
		if(ipq_set_verdict(ipq_handle, m->packet_id, NF_ACCEPT, 0, NULL) < 0) {
			ipq_die(ipq_handle);
		}

	}

	// Will never be executed
	ipq_destroy_handle(ipq_handle);

	return EXIT_SUCCESS;
} //]]
