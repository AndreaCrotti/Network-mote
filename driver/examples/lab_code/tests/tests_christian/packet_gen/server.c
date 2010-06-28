/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */
 	
/**
 * @file udpserver.c
 * @brief Simple UDP server, used for testing connectivity.
 * @author Thomas Jansen <mithi@mithi.net>
 * @date Jul. 2009
 */

#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int main(int argc, char **argv){

	struct sockaddr_in addr = {0};
	size_t len = sizeof(addr);
	char buf[2048];
	int s;

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		return 1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(7777);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *)&addr, len)==-1)
		return 2;

	ssize_t recvbytes;
	while ((recvbytes = recvfrom(s, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *)&addr, &len)) > 0) {
		printf("From %s, Size %u, Content %.*s\n", inet_ntoa(addr.sin_addr), recvbytes, recvbytes, buf);
	}
 	
	return 0;
}
