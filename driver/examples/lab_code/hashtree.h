// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * Hash tree functions for packet authentication and
 * packet signatures
 *
 * Description:
 *
 * Authors:
 *   - Tobias Heer <heer@tobobox.de> 2008
 *   - Rene Hummen <rene.hummen@rwth-aachen.de> 2008
 */

#ifndef HASH_TREE_H_
#define HASH_TREE_H_

#include <openssl/sha.h>
#include <openssl/rand.h>
#include <inttypes.h>

//Define here or rather in your defines.h file
//#define DEBUG_HASHTREE //(already defined in defines.h)
#ifdef DEBUG_HASHTREE
	//need still some hexdump function
	//! \TODO implement hexdump function
	//#define HT_HEXDUMP(tag, data, len) hexdump(data, len);
	#define HT_DEBUG(fmt, args...) printf(fmt, ## args);
	#define HT_ERROR(fmt, args...) \
		printf(fmt, ## args); \
		err = -1; \
		goto out_err;
	#define HT_ERROR_MSG(fmt, args...) printf(fmt, ## args);
	#define HT_HEXDUMP(tag, data, len)
#else
	#define HT_HEXDUMP(tag, data, len)
	#define HT_DEBUG(fmt, args...)
	#define HT_ERROR(fmt, args...) \
		err = -1; \
		goto out_err;
	#define HT_ERROR_MSG(fmt, args...)
#endif
#define _HT_HEXDUMP(tag, data, len)
#define _HT_DEBUG(fmt, args...)

typedef struct htree_gen_args { int index; } htree_gen_args_t;

/* leaf generator function pointer
 * @note if you need more arguments here, add them to the gen_args struct
 */
typedef int (*htree_leaf_gen_t) (const unsigned char *data, int data_length, unsigned char *secret, int secret_length,
	unsigned char *dst_buffer, htree_gen_args_t *gen_args);

typedef int (*htree_node_gen_t) (const unsigned char *left_node, const unsigned char *right_node, int node_length, unsigned char *dst_buffer, htree_gen_args_t *gen_args);

typedef struct hash_tree {

	// data variables
	int num_data_blocks;	/* number of data blocks to be verified with the tree */
	int max_data_length;	/* max length for a single leaf element */
	unsigned char *data;	/* array containing the data to be validated with the tree */
	int *data_length;
	int secret_length;	/* length of the secret */
	unsigned char *secrets;	/* individual secrets to be revealed with each data block */

	struct hash_tree *link_tree;
	int hierarchy_level;

	// tree elements variables
	int node_length;	/* length of a single node element */
	unsigned char *nodes;	/* array containing the nodes of the tree */
	unsigned char *root;	/* the root of the tree -> points into nodes-array */

	// management variables
	int depth;		/* depth of the tree */
	int data_position;	/* index of the next free leaf */
	int is_open;		/* can one enter new entries? This is only true if the nodes have not been computed yet. */

} hash_tree_t;

hash_tree_t* htree_init(int num_data_blocks, int max_data_length, int node_length,
		int secret_length, hash_tree_t *link_tree, int hierarchy_level);

void htree_free(hash_tree_t *tree);

unsigned int htree_get_node_index(const unsigned int level, const unsigned int leaf_count,
	const unsigned int offset);

int htree_add_data(hash_tree_t *tree, const char *data, int data_length);

int htree_add_random_data(hash_tree_t *tree, int num_random_blocks);

int htree_add_secret(hash_tree_t *tree, unsigned char *secret, int secret_length, int secret_index);

int htree_add_random_secrets(hash_tree_t *tree);

int htree_calc_nodes(hash_tree_t *tree, htree_leaf_gen_t leaf_gen,
		htree_node_gen_t node_gen, htree_gen_args_t *gen_args);

int htree_has_more_data(hash_tree_t *tree);

int htree_get_next_data_offset(hash_tree_t *tree);

int htree_get_branch(hash_tree_t *tree, int data_index, unsigned char *branch_nodes, int *branch_length);

unsigned char* htree_get_data(hash_tree_t *tree, int data_index, int *data_length);

unsigned char* htree_get_secret(hash_tree_t *tree, int data_index, int *secret_length);

unsigned char* htree_get_root(hash_tree_t *tree, int *root_length);

int htree_verify_branch(unsigned char *root, int root_length, unsigned char *branch_nodes, uint32_t branch_length, const unsigned char *verify_data,
	int data_length, uint32_t data_index, unsigned char *secret, int secret_length, htree_leaf_gen_t leaf_gen, htree_node_gen_t node_gen,
	htree_gen_args_t *gen_args, unsigned char* output_nodes, unsigned char* output_leaf);

int htree_leaf_generator(unsigned char *data, int data_length, unsigned char *secret, int secret_length, unsigned char *dst_buffer, htree_gen_args_t *gen_args);

int htree_node_generator(unsigned char *left_node, unsigned char *right_node, int node_length, unsigned char *dst_buffer, htree_gen_args_t *gen_args);

void htree_print_data(hash_tree_t *tree);

void htree_print_nodes(hash_tree_t *tree);

double log_x(int base, double value);

#endif /* HASH_TREE_H_ */
