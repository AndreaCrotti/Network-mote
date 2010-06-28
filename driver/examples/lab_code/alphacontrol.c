// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/un.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "defines.h"
#include "alphacontrol.h"

#define TMPDIR "/tmp"
#define print_error(a...) fprintf(stderr, ##a)

static void print_usage(char *me) {
//[[
	printf("Alpha Control -- %s [-t] [-l] [-V]\n", me);
	printf(
		" -t	Tell Alpha to close all connections and terminate\n"
		" -l	Request a list of currently known endpoints\n"
		" -V	Request the Alpha daemons version string\n"
		" -r	Tell alpha do reload its config file\n"
	);
} //]]

/** Send a packet to the alpha unix domain socket
 * @param	sock	The sockets file descriptor
 * @param	buf	The packet
 * @param	len	The length of the packet in bytes
 * @param	to	The sockets address
 * @return	See return value of sendto() is passed through
 */
static int send_internal(int sock, internal_packet *buf, size_t len, struct sockaddr_un *to) {
//[[
	int tolen = sizeof(struct sockaddr_un);
	int ret;

	if((ret = sendto(sock, buf, len, 0, (struct sockaddr*)to, tolen)) == -1) {
		if(errno == EACCES) {
			print_error("You are not allowed to write the socket. (Are you root?)\n");
		} else {
			print_error("sendto(): %s\n", strerror(errno));
		}
	}

	return ret;

} //]]

/** Tell the alpha daemon to terminate
 * @param	sock	The sockets file descriptor
 * @param	to	The sockets address
 * @return	Return code of send_internal()
 */
static int request_term(int sock, struct sockaddr_un *to) {
//[[

	internal_packet packet;
	packet.type = INTERNAL_TERM;
	size_t len = sizeof(internal_packet);

	return send_internal(sock, &packet, len, to);

} //]]

/** Tell the alpha daemon to reload its config file
 * @param	sock	The sockets file descriptor
 * @param	to	The sockets address
 * @return	Return code of send_internal()
 */
static int request_reload(int sock, struct sockaddr_un *to) {
//[[

	internal_packet packet;
	packet.type = INTERNAL_RELOAD;
	size_t len = sizeof(internal_packet);

	return send_internal(sock, &packet, len, to);

} //]]

/** Request the daemons version number
 * @param	sock	The sockets file descriptor
 * @param	to	The sockets address
 * @return	Return code of send_internal()
 */
static int request_version(int sock, struct sockaddr_un *to) {
//[[
	internal_packet packet;
	packet.type = INTERNAL_VERSION;
	return send_internal(sock, &packet, sizeof(packet), to);
} //]]

/** Request a list of known clients
 * @param	sock	The sockets file descriptor
 * @param	to	The sockets address
 * @return	Return code of send_internal()
 */
static int request_client_list(int sock, struct sockaddr_un *to) {
//[[
	internal_packet packet;
	packet.type = INTERNAL_CLIENTLIST;
	return send_internal(sock, &packet, sizeof(packet), to);
} //]]

/** Wait for the client list reply
 * @param	sock	The sockets file descriptor
 * @param	to	The sockets address
 * @return	AP_ERR_SUCCESS or AP_ERR_SOCKET
 */
static ap_err_code wait_for_client_list_reply(int sock, struct sockaddr_un *to) {
//[[

	char buf[INTERNAL_SOCK_MAX_SIZE];
	struct sockaddr_un from;
	socklen_t fromlen = sizeof(struct sockaddr_un);
	int bytes = recvfrom(sock, buf, (size_t)INTERNAL_SOCK_MAX_SIZE, 0, (struct sockaddr*)&from, &fromlen);

	if(bytes == -1) {
		print_error("Failed to receive client list: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	internal_packet_response_client *packet = (internal_packet_response_client*)buf;

	if(packet->type != INTERNAL_CLIENTLIST) {
		print_error("Something went wrong. Wrong packet type.\n");
		return AP_ERR_SOCKET;
	}

	printf("%d endpoints known\n", packet->len);

	int i;
	for(i=0; i<packet->len; i++) {

		struct response_client_data *c = (struct response_client_data*)( (char*)packet + sizeof(internal_packet_response_client) + i * sizeof(struct response_client_data) );

		struct in_addr ip;
		ip.s_addr = c->ip;

		printf("<%d> %s\n", c->id, inet_ntoa(ip));

	}

	return AP_ERR_SUCCESS;

} //]]

/** Wait for the version number reply
 * @param	sock	The sockets file descriptor
 * @param	to	The sockets address
 * @return	AP_ERR_SUCCESS or AP_ERR_SOCKET
 */
static ap_err_code wait_for_version_reply(int sock, struct sockaddr_un *to) {
//[[

	char buf[INTERNAL_SOCK_MAX_SIZE];
	struct sockaddr_un from;
	socklen_t fromlen = sizeof(struct sockaddr_un);
	int bytes = recvfrom(sock, buf, (size_t)INTERNAL_SOCK_MAX_SIZE, 0, (struct sockaddr*)&from, &fromlen);

	if(bytes == -1) {
		print_error("Failed to receive version number: %s\n", strerror(errno));
		return AP_ERR_SOCKET;
	}

	internal_packet *packet = (internal_packet*)buf;

	if(packet->type != INTERNAL_VERSION) {
		print_error("Something went wrong. Wrong packet type.\n");
		return AP_ERR_SOCKET;
	}

	char *ver = (char*)packet + sizeof(internal_packet);

	printf("Alpha daemon version: %s\n", ver);

	return AP_ERR_SUCCESS;

} //]]

int main(int argc, char *argv[]) {

	srand(getpid() * time(NULL));

	if(argc == 1) {
		print_usage(argv[0]);
		return EXIT_SUCCESS;
	}

	int sock;
	if((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
		print_error("Failed to create unix domain socket: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	struct sockaddr_un remote;

	if(strlen(SOCKETFILE)+1 > sizeof(remote.sun_path)) {
		print_error("Failed to create unix domain socket: filename too long.\n");
		return EXIT_FAILURE;
	}

	// We need a temporary local socket, because the server needs to send us data back
	// and we are using datagram sockets
	struct sockaddr_un local;
	local.sun_family = AF_UNIX;
	int filenum = rand() % 10000;
	snprintf(local.sun_path, sizeof(local.sun_path), TMPDIR"/alpha-%d", filenum);

	if(bind(sock, (struct sockaddr*)&local, sizeof(local)) == -1) {
		print_error("Failed to bind local socket: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, SOCKETFILE);

	int c;
	while((c = getopt(argc,argv,"tVlr")) != -1) {
		switch(c) {
			case 't':
				request_term(sock, &remote);
				break;
			case 'V':
				if(request_version(sock, &remote) != AP_ERR_SUCCESS) {
					break;
				}
				wait_for_version_reply(sock, &remote);
				break;
			case 'l':
				if(request_client_list(sock, &remote) != AP_ERR_SUCCESS) {
					break;
				}
				wait_for_client_list_reply(sock, &remote);
				break;
			case 'r':
				request_reload(sock, &remote);
				break;
			default:
				print_usage(argv[0]);
				break;
		}
	}

	// Clean up the temp file
	unlink(local.sun_path);

	return EXIT_SUCCESS;
}
