// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __CACHE_TREE_H_
#define __CACHE_TREE_H_

#include "hashtree.h"
#include <stdbool.h>

//! Enumeration for security modes, i.e. how many nodes are included in each packet
typedef enum cache_tree_security_mode {
	//! one node is included in each packet (low redundancy)
	CACHE_TREE_ONE_SIGNATURE_PER_PACKET = 1,
	//! two nodes are included in each packet (higher redundancy, more overhead)
	CACHE_TREE_TWO_SIGNATURES_PER_PACKET = 2,
} cache_tree_security_mode_t;

unsigned int cache_tree_get_number_of_sent_signatures(const cache_tree_security_mode_t);

unsigned int cache_tree_get_position(const unsigned int, const unsigned int);

void cache_tree_get_pre_send_node(const unsigned int, const unsigned int, unsigned int*, unsigned int*);

void cache_tree_get_nodes(const hash_tree_t*, const unsigned int, const unsigned int, const cache_tree_security_mode_t,
	const unsigned int, unsigned char**);

unsigned char* cache_tree_reallocate_node_buffer(unsigned char* prev_buf, const unsigned int leaf_count, const unsigned int hashsize);

void cache_tree_store_node(unsigned char*, const unsigned char*, const unsigned int, const unsigned int, const unsigned int, const unsigned int);

void cache_tree_get_node(const unsigned char* node_buf, unsigned char* node, const unsigned int position,
	const unsigned int level, const unsigned int leaf_count, const unsigned int hashsize);

void cache_tree_update_buffer(unsigned char*, const unsigned int, const unsigned int, const unsigned char*, const unsigned char*,
	const cache_tree_security_mode_t, const unsigned int);

bool cache_tree_verify_new_cache_entry(const unsigned char*, const unsigned char*, const unsigned char*, const unsigned int,
	const unsigned int, htree_node_gen_t, htree_gen_args_t*, const unsigned int);

void cache_tree_build_verification_buffer(const unsigned char*, const unsigned char*, unsigned char**, const unsigned int,
	const unsigned int, const cache_tree_security_mode_t, const unsigned int);

#endif // __CACHE_TREE_H_
