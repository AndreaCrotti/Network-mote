// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*
 * Hash chain functions for packet authentication and
 * packet signatures
 *
 * Description:
 * In the current version hash-chains created with any hash-function, which
 * output is <= 20 bytes are supported.
 *
 * Authors:
 *   - Tobias Heer <heer@tobobox.de> 2006
 *
 */
#ifndef HASH_CHAIN_H
#define HASH_CHAIN_H

/*
 * Static seeds create reproducable hash chains.
 * This is for testing purposes only!
 */
//#define HACHIN_STATIC_SEED

/* If HASH list is defined, the hash chain implementation will
 * use a real linked list. Otherwise a hash chain array will be
 * used. The interface stays identical.*/
// LIST is broken at the moment (invalid reads in free() function)
// #define HASH_LIST

//Define here or rather in your defines.h file
//#define DEBUG_HASHCHAIN //(already defined in defines.h)
#ifdef DEBUG_HASHCHAIN
	#define HC_HEXDUMP(tag, data, len) hexdump(tag, data, len)
	#define HC_DEBUG(fmt, args...) printf(fmt, ## args)
	#define HC_ERROR(fmt, args...) \
		printf(fmt, ## args); \
		err = -1; \
		goto out_err;
	#define HC_ERROR_MSG(fmt, args...) printf(fmt, ## args);
#else
	#define HC_HEXDUMP(tag, data, len)
	#define HC_DEBUG(fmt, args...)
	#define HC_ERROR(fmt, args...) \
		err = -1; \
		goto out_err;
	#define HC_ERROR_MSG(fmt, args...)
#endif
#define _HC_HEXDUMP(tag, data, len)
#define _HC_DEBUG(fmt, args...)

/* biggest digest in openssl lib */
#ifdef SHA512_DIGEST_LENGTH
 #define MAX_HASH_LENGTH SHA512_DIGEST_LENGTH
#else
 #define MAX_HASH_LENGTH 64
#endif

typedef unsigned char * (*hash_function_t)(const unsigned char *, size_t, unsigned char *);

#ifdef HASH_LIST

typedef struct hash_chain_element {
	unsigned char *hash;
	struct hash_chain_element *next;
} hash_chain_element_t;

typedef struct hash_chain {
	/* pointer to the hash-function used to create and verify the hchain
	 *
	 * @note params: (in_buffer, in_length, out_buffer)
	 * @note out_buffer should be size MAX_HASH_LENGTH */
	hash_function_t hash_function;
	int hash_length;	/* length of the hashes, of which the hchain consist */
	int hchain_length;	/* number of initial elements in the hash-chain */
	int hchain_hierarchy; /* hierarchy this hchain belongs to */
	int remaining;		/* remaining elements int the hash-chain */
	hash_chain_element_t *current_element;
	hash_chain_element_t *source_element;	/* seed - first element */
	hash_chain_element_t *anchor_element;	/* anchor - last element */
} hash_chain_t;

#else
/* Use hash array instead of lists*/

typedef struct hash_chain {
	/* pointer to the hash-function used to create and verify the hchain
	 *
	 * @note params: (in_buffer, in_length, out_buffer)
	 * @note out_buffer should be size MAX_HASH_LENGTH */
	hash_function_t hash_function;
	int hash_length;	/* length of the hashes, of which the hchain consist */
	int hchain_length;	/* number of initial elements in the hash-chain */
	int hchain_hierarchy;	/* hierarchy this hchain belongs to */
	int position;
	unsigned char* hash_field;
} hash_chain_t;

#endif /* HASH_LIST */

void hchain_print(const hash_chain_t * hash_chain);

/* check if a hash is part of a hash chain */
int hchain_verify(const unsigned char * current_hash, const unsigned char * last_hash,
		hash_function_t hash_function, int hash_length, int tolerance);

/* create a new hash chain on the heap */
hash_chain_t * hchain_create(hash_function_t hash_function, int hash_length,
		int hchain_length, int hchain_hierarchy);

/* remove and return the next element from the hash chain */
unsigned char * hchain_pop(hash_chain_t * hash_chain);

/* return the next element from the hash chain */
unsigned char * hchain_next(const hash_chain_t *hash_chain);

/* return the current element from the hash chain */
unsigned char * hchain_current(const hash_chain_t *hash_chain);

/* delete hash chain and free memory */
int hchain_free(hash_chain_t *hash_chain);

int hchain_get_num_remaining(const hash_chain_t *);

void hexdump(const char * const text, const char * const buffer, int length);

#if 0
/*************** Helper functions ********************/
int concat_n_hash_SHA(unsigned char *hash, unsigned char** parts, int* part_length,
		int num_parts);
#endif

#endif /*HASH_CHAIN_H*/
