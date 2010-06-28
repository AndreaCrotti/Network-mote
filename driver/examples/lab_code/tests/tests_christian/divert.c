/*
 * divert.c
 *
 *  Created on: 15.04.2009
 *      Author: heer
 */
#include <stdlib.h> 	/* exit(...) */
#include <stdio.h> 		/* printf(...) */
#include <errno.h> 		/* errno(...) */
#include <netinet/in.h> /* uint_X types */
#include <stdio.h> 		/* printf(...) */
#include <assert.h>		/* assert(...) */
#include <arpa/inet.h>	/* inet_ntoa(...) */

#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


#define MAX_PACKET_SIZE 10000 /* maximum size for a single packet */
#define PORT 880 /** TODO: There are still hard-coded port assignments.*/


typedef enum {ap_prio_high, ap_prio_time, ap_prio_bulk} ap_priority;



void set_rules(){
	/** TODO: This is ugly. Replace! */
	printf("Installing ipfw divert rules.\n");
	system("ipfw add 1000 divert 880 ip from any to me");
	system("ipfw add 1001 divert 880 ip from me to any");


}

void reset_rules(){
	/** TODO: This is ugly. Replace! */
	printf("Removing ipfw divert rules.\n");
	system("ipfw delete 1000");
	system("ipfw delete 1001");
	exit(1);

}

ap_priority ap_classify(struct ip * ip_hdr, int len ) {

	struct tcphdr * tcp_hdr;

	assert(ip_hdr != NULL);

	/* determine protocol */

	switch(ip_hdr->ip_p){
	case IPPROTO_TCP:
		tcp_hdr = (struct tcphdr *) ((char*) ip_hdr + 4* ip_hdr->ip_hl) ;
		/* Some TCP packets should be sent immediately - a problem arising from this
		 * classification could be out-of order delivery.
		 */

		/* There might be no more traffic coming -> send answer soon */
		if(tcp_hdr->th_flags & TH_ACK &&
			 4 * ip_hdr->ip_hl + 4 * (tcp_hdr->th_off) == ntohs(ip_hdr->ip_len)) {
				printf("H TCP: ");
				if(tcp_hdr->th_flags & TH_FIN) printf(" FIN");
				if(tcp_hdr->th_flags & TH_RST) printf(" RST");
				if(tcp_hdr->th_flags & TH_ACK) printf(" ACK");
				if(tcp_hdr->th_flags & TH_URG) printf(" URG");
				if(tcp_hdr->th_flags & TH_SYN) printf(" SYN");
				printf(" Size: %d (TCP: %d)\n",  ntohs(ip_hdr->ip_len) , 4* tcp_hdr->th_off);
				return ap_prio_high;
		}

		/* Opening TCP connections has high priority */
		if(tcp_hdr->th_flags & ( TH_URG | TH_SYN )) {
			printf("H TCP:");
			if(tcp_hdr->th_flags & TH_FIN) printf(" FIN");
			if(tcp_hdr->th_flags & TH_RST) printf(" RST");
			if(tcp_hdr->th_flags & TH_ACK) printf(" ACK");
			if(tcp_hdr->th_flags & TH_URG) printf(" URG");
			if(tcp_hdr->th_flags & TH_SYN) printf(" SYN");
			printf("\n");
			return ap_prio_high;
		}

		/* There might be no more traffic coming -> send answer soon */
		if(tcp_hdr->th_flags & ( TH_FIN | TH_RST )) {
			printf("T TCP:");
			if(tcp_hdr->th_flags & TH_FIN) printf(" FIN");
			if(tcp_hdr->th_flags & TH_RST) printf(" RST");
			if(tcp_hdr->th_flags & TH_ACK) printf(" ACK");
			if(tcp_hdr->th_flags & TH_URG) printf(" URG");
			if(tcp_hdr->th_flags & TH_SYN) printf(" SYN");
			printf("\n");
			return ap_prio_time;
		}

		/* TCP bulk data */
		printf("B TCP: ");
		if(tcp_hdr->th_flags & TH_FIN) printf(" FIN");
		if(tcp_hdr->th_flags & TH_RST) printf(" RST");
		if(tcp_hdr->th_flags & TH_ACK) printf(" ACK");
		if(tcp_hdr->th_flags & TH_URG) printf(" URG");
		if(tcp_hdr->th_flags & TH_SYN) printf(" SYN");
		printf(" Size: %d (TCP: %d)\n",  ntohs(ip_hdr->ip_len) , 4* tcp_hdr->th_off);
		return ap_prio_bulk;

		break;

	/* bulk */

	/* time-bound*/

	case IPPROTO_UDP:
		printf("T: UDP\n");
		return ap_prio_time;
		break;

	/* high priority */
	case IPPROTO_ICMP:
		printf("H ICMP\n");
		return ap_prio_high;
		break;

	default:
		printf("H Proto %d\n",ip_hdr->ip_p);
		return ap_prio_bulk;
	}

}

int main (const int argc,  char * const argv[]) {

	int dsock; /** divert socket **/
	int ret; /** return value **/
	int len; /** length of an incomig packet **/
	struct sockaddr_in bindPort; /* port to which the fw will send the diverted packets */
	struct sockaddr_in incoming_addr; /* address of the incoming packet **/
	unsigned char packet[MAX_PACKET_SIZE]; /** buffer for incoming packets **/
	unsigned int sinlen;
	struct ip * hdr;
	struct in_addr addr;

	printf("Installing divert rules.\n");


	/* register exit handler */
	signal(SIGINT, reset_rules);

	/* Open a divert socket */
	dsock=socket(AF_INET, SOCK_RAW, IPPROTO_DIVERT);
	if (dsock==-1)
	{
		fprintf(stderr,"%s:We could not open a divert socket\n",argv[0]);
		exit(1);
	}

	printf("Binding divert socket to port %d.\n", PORT);

	bindPort.sin_family      = AF_INET;
	bindPort.sin_port        = htons(PORT);
	bindPort.sin_addr.s_addr = 0;

	if(ret = bind(dsock, (struct sockaddr *)&bindPort, sizeof(bindPort)) != 0){
		/* error handling */
		close(dsock);
		fprintf(stderr, "%s: Error bind(): %s",argv[0],strerror(ret));
	}

	set_rules();


	/** process incoming packets **/
	printf("Waiting\n");
	fd_set socks;
	FD_ZERO(&socks);
	FD_SET(dsock,&socks);

	struct timeval timeout;
	int ready=0;
	while(1){
		timeout.tv_sec = 100;
		timeout.tv_usec = 0;

		ready = select(dsock+1, &socks, NULL, NULL, &timeout);
		sinlen=sizeof(struct sockaddr_in);
		len=recvfrom(dsock, packet, MAX_PACKET_SIZE, 0, (struct sockaddr *)&incoming_addr, (socklen_t *)&sinlen);
		hdr=(struct ip *)packet;

		addr=hdr->ip_src;
//		printf("%s: Src addr: %s",argv[0], inet_ntoa(addr));
		addr=hdr->ip_dst;
//		printf(" Dst addr: %s\n", inet_ntoa(addr));
		ap_classify(hdr, len);
		len=sendto(dsock, packet, len ,0, (struct sockaddr *)&incoming_addr, sinlen);
	}



}
