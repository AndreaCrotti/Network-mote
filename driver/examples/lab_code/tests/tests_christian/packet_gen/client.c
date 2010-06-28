/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */ 	
/**
 * @file udpclient.c
 * @brief Simple UDP client, used for testing connectivity.
 * @author Thomas Jansen <mithi@mithi.net>
 * @date Jul. 2009
 */
 	
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>	
#include <time.h>
#include <unistd.h>

int main(int argc, char **argv){
	struct sockaddr_in addr = {0};
	int s;

	if (argc != 2) {
		fprintf(stderr, "usage: udpclient SERVER_IP\n");
		return 3;
	}

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		return 1;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(7777);
	addr.sin_addr.s_addr = inet_addr(argv[1]);
	if (addr.sin_addr.s_addr==-1) {
		fprintf(stderr, "Failed to parse client IP %s\n", argv[1]);
		return 1;
	}
	const unsigned int max_length = 2500;
	const unsigned int min_length = 50;
	const unsigned int step = 100;
	unsigned int length = min_length;
	unsigned int i = 0;
	char *buf = NULL;
	srand(time(NULL));
	for(;;){
		/*length = min_length + (rand() % (max_length - min_length));
		buf = realloc(buf, length);
		for(i = 0; i < length; ++i){
			buf[i] = rand() % 255;		
		}*/

		length = 1450; //min_length + ((length + step - min_length) % (max_length - min_length));
		buf = realloc(buf, length);
		for(i = 0; i < length; ++i){
			buf[i] = 65 + (i % 26);		
		}
		printf("Sending packet with size %u\n", length);
		sendto(s, buf, length, MSG_EOR, (struct sockaddr *)&addr , sizeof(addr));
		usleep(1000 * 1000);
	}
	free(buf);
	
 	return 0;
}



