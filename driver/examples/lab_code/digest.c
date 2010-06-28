// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	digest.c
 * @brief	tools and helper functions for dealing with hash-chains and MACs
 */

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <string.h>
#include "digest.h"
#include "defines.h"
#include "xmalloc.h"

/** Calculate hash to send in RETURN_CONNECT
 * @param	connect_secret		global connect secret
 * @param	fromaddr		Address of connecting peer
 * @param	timeslot		Timeslot for valid packet
 * @param	digest			Pointer for storing the hash
 * @return	Pointer to the hash
 */
unsigned char* connect_hash(int connect_secret, struct in_addr fromaddr, int timeslot, unsigned char *digest) {
//[[
	unsigned char *blob;
	size_t blobsize = sizeof(int)*2 + sizeof(struct in_addr);
	blob = xmalloc(blobsize);

	memcpy(blob, &connect_secret, sizeof(int));
	memcpy(blob+sizeof(int), &timeslot, sizeof(int));
	memcpy(blob+sizeof(int)*2, &fromaddr, sizeof(struct in_addr));

	unsigned char *retval;
	retval = create_digest(blob, blobsize, digest);
	xfree(blob);
	//statusmsg("DEBUG: connect_hash with addr %s, timeslot %d and digest %.*s\n", inet_ntoa(fromaddr), timeslot, DIGEST_PRINTLEN, digeststr(digest, NULL));
	return retval;
} //]]

/** Computes the n-th element of a hash-chain using the initial secret
 * @param	secret		The initial secret
 * @param	hash_rounds	Position of element in hash-chain
 * @param	digest		Place to store the digest/anchor
 * @return	Pointer to the digest/anchor
 */
unsigned char* hash_chain(unsigned const char* secret, unsigned int hash_rounds, unsigned char* digest) {
//[[
	// TODO: Right now this only supports seed-values of HASHSIZE
	int i=0;
	unsigned char tmp_digest[HASHSIZE];
	memcpy(tmp_digest, secret, HASHSIZE);			// copy the seed/secret to the temp store

	for (i=hash_rounds; i>0; i--) {
		create_digest(tmp_digest, HASHSIZE, digest);	// compute hash of temp store and store it in digest
		memcpy(tmp_digest, digest, HASHSIZE);		// copy digest to temp store for next round of hashing
	}

	memcpy(digest, tmp_digest, HASHSIZE);			// copy digest even if no rounds were computed

	return digest;
}//]]

/** Create the digest of a block of data
 * @param	buffer		Data to digest
 * @param	buffer_len	Length of data to read from buffer
 * @param	digest		Place to store the digest (HASHSIZE)
 * @return	Char-pointer to digest
 */
unsigned char* create_digest(const unsigned char* buffer, size_t buffer_len, unsigned char* digest) {
//[[
	return SHA1(buffer,buffer_len,digest);
}//]]

/** Create a standard-conform HMAC of a block of data keyed with a key
 * @param	buffer		Data to digest
 * @param	buffer_len	Length of data to read from buffer
 * @param	key		Key to use
 * @param	key_len		Size of key
 * @param	digest		Place to store the digest (HASHSIZE)
 * @return	Char-pointer to computed MAC
 */
unsigned char* hmac(const void* buffer, unsigned int buffer_len, const void* key, unsigned int key_len, void* digest) {
//[[
	 return HMAC(EVP_sha1(),key, key_len, buffer, buffer_len, digest, NULL);
} //]]

/** Provides an easy way to print out the digest of a hash (i.e. bytes)
 ** (NOTE: this function does not actually print anything, it just returns a char* which then
 **  CAN be printed (using printf or something else ...)
 * @param	hash_store	char array where the hash is stored
 * @param	output_buffer	char array where the output should be stored or NULL, if static storage should be used
 * @return	pointer to output_buffer, so function can be used in printf
 */
char* digeststr(const unsigned char* hash_store, char* output_buffer) {
//[[
	int i;
	char tmp = 0;

	static char static_buffer[2*HASHSIZE+1];
	char *buf = (output_buffer != NULL) ? output_buffer : static_buffer;

	for(i=0; i<HASHSIZE; i++) {

		tmp = hash_store[i] / 16;
		buf[ 2*i   ] = tmp + ((tmp > 9) ? ('a'-10) : '0');

		tmp = (char)(hash_store[i] % 16);
		buf[(2*i)+1] = tmp + ((tmp > 9) ? ('a'-10) : '0');

	}

	// null-terminate the string
	buf[2*HASHSIZE] = 0;

	return buf;
}//]]
