// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#define INTERNAL_SOCK_MAX_SIZE	100

#define INTERNAL_VERSION	1
#define INTERNAL_TERM		2
#define INTERNAL_CLIENTLIST	3
#define INTERNAL_RELOAD		4

typedef struct {
	uint8_t		type;
} __attribute__((__packed__)) internal_packet;

typedef struct {
	uint8_t		type;
	uint8_t		len;	// number of client structs following
	// client structs follow
} __attribute__((__packed__)) internal_packet_response_client;

struct response_client_data {
	uint8_t		id;
	uint32_t	ip;
	// TODO: fill me wiht more useful stuff
};
