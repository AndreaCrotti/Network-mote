// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __DIGEST_H
#define __DIGEST_H

#include <netinet/in.h>

unsigned char* connect_hash(int connect_secret, struct in_addr fromaddr, int timeslot, unsigned char *digest);
unsigned char* hash_chain(unsigned const char *secret, unsigned int hash_rounds, unsigned char *digest);
unsigned char* create_digest(const unsigned char *buffer, size_t buffer_len, unsigned char *digest);
unsigned char* hmac(const void *buffer, unsigned int buffer_len, const void *key, unsigned int key_len, void *digest);
char* digeststr(const unsigned char *hash_store, char *output_buffer);

#endif // __DIGEST_H
