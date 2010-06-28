// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	key.c
 * @brief	helper functions for dealing with DSA keys
 */

#include <openssl/dsa.h>
#include <openssl/md5.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "key.h"

// Read DSA datastructure into a simple blob
static unsigned char* key_to_blob(const DSA *key, size_t *blobsize) {
//[[
	assert(key && blobsize);

	unsigned char *blob;
	*blobsize = key->p->top + key->q->top + key->g->top + key->pub_key->top + 8;
	blob = xmalloc(*blobsize);

	// TODO: In order to achieve the same output as ssh-keygen we would have to
	// convert and copy the bignums and use padding bytes like OpenSSH does with its buffers
	// see bignum_st in openssl/bn.h and buffer_put_bignum_ret in bufbn.c of OpenSSH

	memcpy(blob,							"ssh-dss",		8);
	memcpy(blob+8, 							key->p->d, 		key->p->top);
	memcpy(blob+8+key->p->top, 					key->q->d, 		key->q->top);
	memcpy(blob+8+key->p->top+key->q->top,				key->g->d, 		key->g->top);
	memcpy(blob+8+key->p->top+key->q->top+key->g->top,		key->pub_key->d,	key->pub_key->top);

	return blob;
} //]]

// Digest the key in a blob 
static unsigned char* digest_blob(unsigned char *blob, size_t len, unsigned char *digest) {
//[[
	assert(blob && digest);

	// since we might try to achieve compatibility to openssh fingerprints
	// we will use MD5, just like OpenSSH does by default
	return MD5(blob,len,digest);
} //]]

// Fingerprint a DSA key and save the output 
char* key_fingerprint(const DSA *key, char *output) {
//[[
	static char fp_buffer[MD5_DIGEST_LENGTH*3+1];	// we want xx:xx:... format, so times 3!
	char *retval = (output) ? output : fp_buffer;

	if(!key) {
		strncpy(retval, "<no valid DSA key>", 19);
		return retval;
	}

	unsigned char *blob = NULL;
	size_t blobsize;
	blob = key_to_blob(key, &blobsize);

	unsigned char digest[MD5_DIGEST_LENGTH];
	digest_blob(blob, blobsize, digest);
	xfree(blob);

	int i;
	for (i = 0; i < MD5_DIGEST_LENGTH; i++) {
		char hex[4];
		snprintf(hex, sizeof(hex), "%02x:", digest[i]);
		memcpy(retval+(3*i),hex,sizeof(hex));
	}

	retval[(MD5_DIGEST_LENGTH * 3) - 1] = '\0';	// Remove the trailing ':' character

	return retval;
} //]]
