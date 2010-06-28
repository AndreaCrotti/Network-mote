// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	hashchain.c
 * @brief	Data-structures, constructors and accessors for the hash_chain
 */

#include <string.h>
#include <assert.h>
#include <openssl/rand.h>
#include <stdio.h>

#include "hashchain.h"
#include "tools.h"
#include "xmalloc.h"
#include "digest.h"

#ifndef HASH_LIST
void hchain_print(const hash_chain_t * hchain) {
//[[

	if(hchain) {
		HC_DEBUG("Hash chain: %p\n", hchain);

		HC_HEXDUMP("currrent element: ", (char *) hchain->hash_field + hchain->position * hchain->hash_length,
				hchain->hash_length);

		HC_DEBUG("Remaining elements: %d\n", hchain->position);
		HC_DEBUG(" - Contents:\n");

		int i;
		for (i = hchain->hchain_length-1; i >= 0; i--)
		{
			if(i < hchain->position) {
				HC_DEBUG("(+) element %i:", i + 1);
			} else {
				HC_DEBUG("(-) element %i:", i + 1);
			}

			HC_HEXDUMP("\t", (char *) hchain->hash_field+ i * hchain->hash_length, hchain->hash_length);
			HC_DEBUG("\n");
		}
	} else {
		HC_DEBUG("Given hash chain was NULL!\n");
	}
} //]]

/**
 * hchain_verify - verify if the given hash is part of a hash chain
 * @current_hash: the given hash value
 * @last_hash: the last known hash value
 * @tolerance: The tolerance limit determines how many steps may be missing in the hash chain
 *             0 means that only sequential hash values are considered as valid.
 * @return: returns hash distance if the hash authentication was successful, 0 otherwise
 */
int hchain_verify(const unsigned char * current_hash, const unsigned char * last_hash,
		hash_function_t hash_function, int hash_length, int tolerance) {
//[[
	/* stores intermediate hash results */
	unsigned char buffer[MAX_HASH_LENGTH];
	int err = 0, i;

	assert(current_hash != NULL && last_hash != NULL);
	assert(hash_function != NULL);
	assert(hash_length > 0 && tolerance >= 0);

	// init buffer with the hash we want to verify
	memcpy(buffer, current_hash, hash_length);

	_HC_HEXDUMP("comparing given hash: ", (char *) buffer, hash_length);
	_HC_DEBUG("\t<->\n");
	_HC_HEXDUMP("last known hash: ", (char *) last_hash, hash_length);

	for (i = 1; i <= tolerance; i++) {
		_HC_DEBUG("Calculating round %i:\n", i);

		hash_function(buffer, hash_length, buffer);

		_HC_HEXDUMP("comparing buffer: ", (char *) buffer, hash_length);
		_HC_DEBUG("\t<->\n");
		_HC_HEXDUMP("last known hash: ", (char *) last_hash, hash_length);

		// compare the elements
		if(!(memcmp(buffer, last_hash, hash_length))) {
			HC_DEBUG("hash verfied\n");

			err = i;
			goto out_err;
		}
	}

	HC_DEBUG("no matches found within tolerance: %i!\n", tolerance);

	out_err:
		return err;
} //]]

hash_chain_t * hchain_create(hash_function_t hash_function, int hash_length,
		int hchain_length, int hchain_hierarchy) {
//[[
	hash_chain_t *return_hchain = NULL;

	assert(hash_function != NULL);
	// make sure that the hash we want to use is smaller than the max output
	assert(hash_length > 0 && hash_length <= MAX_HASH_LENGTH);
	assert(hchain_length > 0);

	/* the hash function output might be longer than needed
	 * allocate enough memory for the hash function output */
	register unsigned char* hash_value;
	register unsigned char* previous_value;
	hash_value = xmalloc(MAX_HASH_LENGTH);
	previous_value = xmalloc(MAX_HASH_LENGTH);

	// allocate memory for a new hash chain and set members to 0/NULL
	return_hchain = xmalloc(sizeof(hash_chain_t));

	memset(return_hchain, 0, sizeof(hash_chain_t));

	/* allocate memory for the hashes */
	return_hchain->hash_field = xmalloc(hash_length * hchain_length);

	// Fill up the chain from front to back
	unsigned char* current_element = return_hchain->hash_field;
	int i;

	// First element by random input
	RAND_bytes(current_element, hash_length);
	memcpy(previous_value, current_element, hash_length);

	for(i = 1; i < hchain_length; i++) {

		// (input, input_length, output) -> output_length == 20
		hash_function(previous_value, hash_length, hash_value);
		
		// only consider DEFAULT_HASH_LENGTH highest bytes
		memcpy(current_element+(i*hash_length), hash_value, hash_length);
		memcpy(previous_value, hash_value, hash_length);
	}

	return_hchain->hash_function = hash_function;
	return_hchain->hash_length = hash_length;
	return_hchain->hchain_length = hchain_length;
	return_hchain->hchain_hierarchy = hchain_hierarchy;
	return_hchain->position = hchain_length-1;

	HC_DEBUG("Hash-chain with %i elements of length %i created!\n", hchain_length, hash_length);

	xfree(hash_value);
	xfree(previous_value);

	return return_hchain;

} //]]

/**
 * hchain_pop - return the next element in a hash chain
 * 		and move the current_element pointer forward
 * @hchain: the hash chain which has to be popped
 * @return: pointer to the current hash_chain element
 */
unsigned char * hchain_pop(hash_chain_t * hchain) {
//[[
	unsigned char* element = NULL;

	assert(hchain != NULL);

	if(hchain->position <= 0) {
		HC_ERROR_MSG("hchain_next: Hash chain depleted!\n");
		return NULL;
	}

	hchain->position--;
	element = hchain->hash_field + hchain->position * hchain->hash_length;

	HC_HEXDUMP("Popping hash chain element: ", (char *) element, hchain->hash_length);
	HC_DEBUG("\n");

	return element;
} //]]

/**
 * hchain_next - returns the next element of the hash chain but does not advance the current_element
 * pointer. This function should only be used if the next element is kept secret and has to
 * be used for special puroses like message signatures.
 * @hchain: the hash chain
 * @return: next element of the hash chain or NULL if the hash chain is depleted.
 */
unsigned char * hchain_next(const hash_chain_t *hchain) {
//[[

	unsigned char* next_hash = NULL;
	assert(hchain != NULL);

	if(hchain->position <= 0) {
		print_error("hchain_next: Hash chain depleted!\n");
		return NULL;
	} else {
		// hash chain in use: return next.
		next_hash = hchain->hash_field + (hchain->position - 1) * hchain->hash_length;
#ifdef DEBUG_HASHCHAIN
		statusmsg("<?> Next hash chain element %.*s...\n", DIGEST_PRINTLEN, digeststr((unsigned char *) next_hash, NULL));
#endif
		return next_hash;
	}
} //]]

/**
 * hchain_current - returns the current element of the hash chain
 * @hchain: the hash chain
 * @return: current element of the hash chain or NULL if the hash chain is depleted.
 */
unsigned char * hchain_current(const hash_chain_t *hchain) {
//[[
	unsigned char *current_hash = NULL;

	assert(hchain != NULL);
	assert(hchain->position <= hchain->hchain_length);

	current_hash = hchain->hash_field + hchain->position * hchain->hash_length;

#ifdef DEBUG_HASHCHAIN
	statusmsg("<?> Current hash chain element %.*s...\n", DIGEST_PRINTLEN, digeststr((unsigned char *) current_hash, NULL));
#endif

	return current_hash;

} //]]

/**
 * hchain_destruct - delete hash chain and free memory
 * @hchain: the hash chain which has to be removed
 * @return: 0 in case of success
 */
int hchain_free(hash_chain_t *hchain) {
//[[

	if(hchain != NULL) {
		assert(hchain->hash_field != NULL);
		xfree(hchain->hash_field);
		xfree(hchain);
	}

	HC_DEBUG("All hash-chain elements freed\n");
	return 0;
} //]]

/**
 * hchain_get_num_remaining - accessor function which returns the number of remaining hash chain
 * elements
 * @hchain: the hash chain
 * @return: number of remaining elements
 **/
int hchain_get_num_remaining(const hash_chain_t * hchain) {
//[[
	assert(hchain != NULL);
	return hchain->hchain_length - hchain->position;
} //]]

/**
* hexdump - prints a string as hexadecimal characters.
 * @buffer: buffer to print
 * @length: Length of the buffer (in bytes)
 */
void hexdump(const char * const text, const char * const buffer, int length) {
//[[
	if(buffer == NULL) {
		printf("hexdump: NULL BUFFER GIVEN!!!!\n");
		printf("%s, buff %p", text, (const void*) buffer);
	} else {
		int i;
		printf("%s", text);
		for(i = 0; i < length; i++) {

			printf("%02X", (unsigned char) buffer[i]);
		}
	}
} //]]

#else /* ifdef HASH_LIST, should be duplicate of the above function sigs */
//[[

// these are not needed and therefore not implemented
// right now but they should be used where necessary
#define HCHAIN_LOCK(lock_id)
#define HCHAIN_UNLOCK(lock_id)

void hchain_print(const hash_chain_t * hash_chain) {
//[[
	hash_chain_element_t *current_element = NULL;
	int i;

	if(hash_chain) {
		HC_DEBUG("Hash chain: %d\n", (int) hash_chain);

		if(hash_chain->current_element) {
			HC_HEXDUMP("currrent element: ", (char *) hash_chain->current_element->hash,
					hash_chain->hash_length);
		} else {
			HC_DEBUG(" -- hash chain not in use -- \n");
		}

		HC_DEBUG("Remaining elements: %d\n", hchain_get_num_remaining(hash_chain));
		HC_DEBUG(" - Contents:\n");

		for (current_element = hash_chain->anchor_element, i=0;
				current_element != NULL;
				current_element = current_element->next, i++)
		{
			if(hash_chain->hchain_length - hash_chain->remaining < i+1) {
				HC_DEBUG("(+) element %i:", i + 1);
			} else {
				HC_DEBUG("(-) element %i:", i + 1);
			}

			HC_HEXDUMP("\t", (char *) current_element->hash, hash_chain->hash_length);
			HC_DEBUG("\n");
		}
	} else {
		HC_DEBUG("Given hash chain was NULL!\n");
	}
} //]]

/**
 * hchain_create - create a new hash chain of a certain length
 * @param hash_function Pointer to hash function f
 * @param hash_length Output size of Function f in bytes
 * @param hchain_length: number of hash entries
 * @param hchain_hierachy number of levels in hash chain
 * @return: returns a pointer to the newly created hash_chain
 */
hash_chain_t * hchain_create(hash_function_t hash_function, int hash_length,
		int hchain_length, int hchain_hierarchy) {
//[[
	hash_chain_t *return_hchain = NULL;
	hash_chain_element_t *last_element = NULL, *current_element = NULL;
	unsigned char *hash_value = NULL;
	int i, err = 0;

	assert(hash_function != NULL);
	// make sure that the hash we want to use is smaller than the max output
	assert(hash_length > 0 && hash_length <= MAX_HASH_LENGTH);
	assert(hchain_length > 0);

	/* the hash function output might be longer than needed
	 * allocate enough memory for the hash function output */
	hash_value = xmalloc(MAX_HASH_LENGTH);

	// allocate memory for a new hash chain and set members to 0/NULL
	return_hchain = xmalloc(sizeof(hash_chain_t));
	memset(return_hchain, 0, sizeof(hash_chain_t));

	for(i = 0; i < hchain_length; i++) {
		// reuse memory for hash-value buffer
		memset(hash_value, 0, MAX_HASH_LENGTH);

		// allocate memory for a new element
		current_element = xmalloc(sizeof(hash_chain_element_t));
		current_element->hash = xmalloc(hash_length);

		if(last_element != NULL) {
			// (input, input_length, output) -> output_length == 20
			if(!(hash_function(last_element->hash, hash_length, hash_value))) {
				HC_ERROR("failed to calculate hash in hchain_create\n");
			}
			// only consider DEFAULT_HASH_LENGTH highest bytes
			memcpy(current_element->hash, hash_value, hash_length);
		} else {

#ifdef HACHIN_STATIC_SEED
		#warning Hash chain uses static seed. Whatever you do, it will be INSECURE
			// static seed
			if(memset(current_element->hash, 1, hash_length) <= 0) {
				HC_ERROR("failed to set static seed\n");
			}

#else
			// random bytes as seed
			if(RAND_bytes(current_element->hash, hash_length) <= 0) {
				HC_ERROR("failed to get random bytes for source element int hchain_create\n");
			}
#endif

			return_hchain->source_element = current_element;
		}

		_HC_HEXDUMP("element created: ", current_element->hash, hash_length);

		// list with backwards links
		current_element->next = last_element;
		last_element = current_element;
	}

	return_hchain->hash_function = hash_function;
	return_hchain->hash_length = hash_length;
	return_hchain->hchain_length = hchain_length;
	return_hchain->remaining = hchain_length;
	return_hchain->hchain_hierarchy = hchain_hierarchy;
	// hash_chain->source_element set above
	return_hchain->anchor_element  = current_element;
	return_hchain->current_element = NULL;

	HC_DEBUG("Hash-chain with %i elements of length %i created!\n", hchain_length, hash_length);
	// hchain_print(return_hchain, hash_length);
	// HIP_IFEL(!(hchain_verify(return_hchain->source_element->hash, return_hchain->anchor_element->hash,
	//		hash_function, hash_length, hchain_length)), -1, "failed to verify the hchain\n");
	// HC_DEBUG("hchain successfully verfied\n");

	out_err:
		if(err) {
			// try to free all that's there
			if(return_hchain->anchor_element) {
				// hchain was fully created
				hchain_free(return_hchain);
			} else {
				while (current_element) {
					last_element = current_element;
					current_element = current_element->next;
					xfree(last_element);
				}
				if(return_hchain->source_element)
					xfree(return_hchain->source_element);
			}

			if(return_hchain)
				xfree(return_hchain);
			return_hchain = NULL;
		}

// normal clean-up
	if(hash_value)
		xfree(hash_value);

	return return_hchain;
} //]]

/**
 * hchain_pop - return the next element in a hash chain
 * 		and move the current_element pointer forward
 * @hash_chain: the hash chain which has to be popped
 * @return: pointer to the current hash_chain element
 */
unsigned char * hchain_pop(hash_chain_t * hash_chain) {
//[[
	int err = 0;
	hash_chain_element_t *tmp_element = NULL;
	unsigned char *popped_hash = NULL;

	assert(hash_chain != NULL);

	HCHAIN_LOCK(&hash_chain);
	if(hash_chain->current_element != NULL) {
		// hash chain already in use
		if(hash_chain->current_element->next == NULL) {
			HC_ERROR_MSG("hchain_next: Hash chain depleted!\n");
			exit(1);
		} else {
			tmp_element = hash_chain->current_element->next;
		}
	} else {
		// hash_chain unused yet
		tmp_element = hash_chain->anchor_element;
	}

	popped_hash = tmp_element->hash;

	HC_HEXDUMP("Popping hash chain element: ", (char *) popped_hash, hash_chain->hash_length);
	HC_DEBUG("\n");

	// hchain update
	hash_chain->current_element = tmp_element;
	hash_chain->remaining--;

	// removes the warning due to not used jump label
	goto out_err;

	out_err:
		HCHAIN_UNLOCK(&hash_chain);

		if(err)
			popped_hash = NULL;

	return popped_hash;
} //]]

unsigned char * hchain_next(const hash_chain_t *hash_chain) {
//[[
	unsigned char *next_hash = NULL;
	int err = 0;

	assert(hash_chain != NULL);

	if(hash_chain->current_element != NULL) {
		// hash chain already in use
		if( hash_chain->current_element->next == NULL ) {
			// hash chain depleted. return NULL
			HC_ERROR_MSG("hchain_next: Hash chain depleted!\n");
			exit(1);
		} else {
			// hash chain in use: return next.
			next_hash = hash_chain->current_element->next->hash;
		}
	} else {
		// hash_chain is unused. return the anchor element
		next_hash = hash_chain->anchor_element->hash;
	}

	HC_HEXDUMP("Next hash chain element: ", (char *) next_hash, hash_chain->hash_length);
	HC_DEBUG("\n");
	// removes the warning due to not used jump label
	goto out_err;
	out_err:
	if(err)
		next_hash = NULL;

	return next_hash;
} //]]

unsigned char * hchain_current(const hash_chain_t *hash_chain) {
//[[
	unsigned char *current_hash = NULL;
	int err = 0;

	assert(hash_chain != NULL);
	assert(hash_chain->current_element != NULL);

	current_hash = hash_chain->current_element->hash;

	// HC_HEXDUMP("Current hash chain element: ", (char *) current_hash, hash_chain->hash_length);

#ifdef DEBUG_HASHCHAIN
	statusmsg("<?> Current hash chain element %.*s...\n", DIGEST_PRINTLEN, digeststr((unsigned char *) current_hash, NULL));
#endif

	// removes the warning due to not used jump label
	goto out_err;

	out_err:
		if(err)
			current_hash = NULL;

	return current_hash;
} //]]

int hchain_free(hash_chain_t *hash_chain) {
//[[
	hash_chain_element_t *current_element = NULL; /* the element to be deleted */
	hash_chain_element_t *next_element    = NULL; /* the next element taht should be deleted */

	int err = 0;

	if(hash_chain) {
		/* start with the anchor */
		next_element = hash_chain->anchor_element;
		while(next_element) {

			/* we are talking about the current element now */
			current_element = next_element;
			/* figure out the next element */
			next_element = current_element->next;

			/* delete the current element */
			xfree(current_element->hash);
			xfree(current_element);
		}

		xfree(hash_chain);
	}

	HC_DEBUG("all hash-chain elements freed\n");

	// removes the warning due to not used jump label
	goto out_err;
	out_err:
		return err;
} //]]

int hchain_get_num_remaining(const hash_chain_t * hash_chain) {
//[[
	assert(hash_chain != NULL);
	return hash_chain->remaining;
} //]]

#endif //]]
