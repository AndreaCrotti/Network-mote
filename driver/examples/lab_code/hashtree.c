// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @author:	Tobias Heer <heer@tobobox.de> 2008
 * @author:	Rene Hummen <rene.hummen@rwth-aachen.de> 2008
 */

// TODO: Muss das sein? floor(), etc. geht sicher auch effizienter als ueber libmath
#include <math.h>

#include <assert.h>
#include <string.h>

#include "hashtree.h"
#include "xmalloc.h"

/** creates an empty hash tree.
 * @param	num_data_blocks number of leaf node
 * @param	max_data_length the maximum data length hashed in a leaf node
 * @param	node_length the length of a hash value
 * @param	secret_length length of the eventual secrets
 * @param	link_tree the link tree in case of HHL-based linking
 * @param	hierarchy_level the hierarchy level of the created hash tree
 * @return	pointer to the tree, NULL in case of an error.
 */
hash_tree_t* htree_init(int num_data_blocks, int max_data_length, int node_length, int secret_length, hash_tree_t *link_tree, int hierarchy_level) {
//[[
	hash_tree_t *tree = NULL;
	int i;
	int err = 0;

	// check here that it's a power of 2
	assert(num_data_blocks > 0 && floor(log_x(2, num_data_blocks)) == ceil(log_x(2, num_data_blocks)));
	assert(max_data_length > 0);
	assert(node_length > 0);

	// allocate the memory for the tree
	tree = xmalloc(sizeof(hash_tree_t));
	bzero(tree, sizeof(hash_tree_t));
	tree->data = xmalloc(num_data_blocks * max_data_length);

	// a binary tree with n leafs has got 2n-1 total nodes
	tree->nodes = xmalloc(node_length * num_data_blocks * 2);

	// if link_tree is set, overwrite secret_length
	if(link_tree) {
		HT_DEBUG("link_tree set\n");
		secret_length = link_tree->node_length;
	}

	// init array elements to 0
	bzero(tree->data, num_data_blocks * max_data_length);
	bzero(tree->nodes, node_length * num_data_blocks * 2);

	tree->is_open = 1;
	tree->data_position = 0;
	tree->num_data_blocks = num_data_blocks;
	tree->max_data_length = max_data_length;
	tree->data_length = xmalloc(sizeof(int) * num_data_blocks);
	tree->node_length = node_length;
	tree->secret_length = secret_length;
	tree->depth = ceil(log_x(2, num_data_blocks));

	// set the link tree
	tree->link_tree = link_tree;
	tree->hierarchy_level = hierarchy_level;

	HT_DEBUG("tree->depth: %i\n", tree->depth);

	tree->root = NULL;

	// now we can init the secret array
	if(secret_length > 0) {

		tree->secrets = xmalloc(secret_length * num_data_blocks);

		if(link_tree) {
			// add the root as secret for each leaf
			for (i = 0; i < num_data_blocks; i++) {
				if(htree_add_secret(tree, link_tree->root, secret_length, i)) {
					HT_ERROR("failed to add linking root as secrets\n");
				}
			}

		} else {
			bzero(tree->secrets, secret_length * num_data_blocks);
		}
	}

out_err:
	if(err) {
		htree_free(tree);
	}

return tree;
} //]]

/** frees the hash tree
 * @param	tree the hash tree to be freed
 */
void htree_free(hash_tree_t *tree) {
//[[
	if(tree) {
		if(tree->nodes)
			xfree(tree->nodes);
		if(tree->data)
			xfree(tree->data);
		if(tree->secrets)
			xfree(tree->secrets);
		if(tree->data_length)
			xfree(tree->data_length);
		xfree(tree);
	}

	tree = NULL;
} //]]

//!
/*!
 *  @param[in] level the level in which the node is
 *  @param[in] leaf_count the number of leafs in the tree
 *  @param[in] offset the position of the node in the given level
 *  @return value of the position of the node in the array (without multipled hashsize)
 */
unsigned int htree_get_node_index(const unsigned int level, const unsigned int leaf_count,
	const unsigned int offset) {
//[[
	return (2 * leaf_count - (leaf_count / (1 << (level-1)))) + offset;
} //]]

/** adds a data item to the tree.
 * @param	tree pointer to the tree
 * @param 	data the data to be added
 * @param	data_length length of the data item
 * @return	always 0
 */
int htree_add_data(hash_tree_t *tree, const char *data, int data_length) {
//[[
	assert(tree != NULL);
	assert(data != NULL);
	assert(data_length > 0 && data_length <= tree->max_data_length);
	assert(tree->is_open > 0);
	assert(tree->data_position < tree->num_data_blocks);

	/* add the leaf the leaf-array
	*
	* @note data_length < tree->max_data_length will result in 0 bits padding
	*/
	memcpy(&tree->data[tree->data_position * tree->max_data_length], data, data_length);
	tree->data_length[tree->data_position] = data_length;
	// move to next free position
	tree->data_position++;
	HT_DEBUG("added data block\n");

	// close the tree, if it is full
	if(tree->data_position == tree->num_data_blocks) {
		HT_DEBUG("tree is full! closing...\n");
		tree->is_open = 0;
		tree->data_position = 0;
	}

	return 0;
} //]]

/** adds random data item to the tree.
 * @param	tree pointer to the tree
 * @param	num_random_blocks number of random blocks to be added
 * @return	always 0
 */
int htree_add_random_data(hash_tree_t *tree, int num_random_blocks) {
//[[
	assert(tree != NULL);
	assert(num_random_blocks > 0);
	assert(tree->is_open > 0);
	assert(tree->data_position + num_random_blocks <= tree->num_data_blocks);

	// add num_random_blocks random data to the data-array
	RAND_bytes(&tree->data[tree->data_position * tree->max_data_length],
		num_random_blocks * tree->max_data_length);
	// move to next free position
	tree->data_position += num_random_blocks;
	HT_DEBUG("added random data block\n");

	// close the tree, if it is full
	if(tree->data_position == tree->num_data_blocks) {
		HT_DEBUG("tree is full! closing...\n");
		tree->is_open = 0;
		tree->data_position = 0;
	}

	return 0;
} //]]

/** adds a secret to the tree.
 * @param	tree pointer to the tree
 * @param 	secret the secret to be added
 * @param	secret_length length of the secret
 * @param	secret_index position of the secret in the leaf set
 * @return	always 0
 */
int htree_add_secret(hash_tree_t *tree, unsigned char *secret, int secret_length, int secret_index) {
//[[
	assert(tree != NULL);
	assert(secret != NULL);
	assert(secret_length == tree->secret_length);
	assert(tree->is_open > 0);

	memcpy(&tree->secrets[secret_index * secret_length], secret, secret_length);
	_HT_DEBUG("added secret block\n");

	return 0;
} //]]

/** adds random secrets to the tree.
 * @param	tree pointer to the tree
 * @return	always 0
 */
int htree_add_random_secrets(hash_tree_t *tree) {
//[[
	int err = 0;

	assert(tree != NULL);
	assert(tree->secrets != NULL);
	assert(tree->secret_length > 0);

	// add num_random_blocks random data to the data-array
	RAND_bytes(&tree->secrets[0], tree->num_data_blocks * tree->secret_length);

	HT_DEBUG("random secrets added\n");

	// prevent compiler warning
	goto out_err;
	out_err:
		return err;
} //]]

/** generates the nodes for a tree with completely filled leaf set
 * @param	tree pointer to the tree
 * @param	leaf_gen the leaf generator function pointer
 * @param	node_gen the node generator function pointer
 * @param	gen_args arguments for the generators
 * @return	0 on success, -1 otherwise
 */
int htree_calc_nodes(hash_tree_t *tree, htree_leaf_gen_t leaf_gen,
		htree_node_gen_t node_gen, htree_gen_args_t *gen_args) {
//[[
	int level_width = 0, i, err = 0;
	// first leaf to be used when calculating next tree level in bytes
	int source_index = 0;
	int target_index = 0;
	unsigned char *secret = NULL;

	assert(tree != NULL);
	// tree has to be full
	assert(tree->is_open == 0);
	assert(tree->data_position == 0);

	/* traverse all data blocks and create the leafs */
	HT_DEBUG("computing leaf nodes: %i\n", tree->num_data_blocks);

	for(i = 0; i < tree->num_data_blocks; i++) {
		_HT_DEBUG("calling leaf generator function...\n");

		// only use secrets if they are defined
		if(tree->secret_length > 0)
			secret = &tree->secrets[i * tree->secret_length];

		// input: i-th data block -> output as i-th node-array element
		if(leaf_gen(&tree->data[i * tree->max_data_length], tree->data_length[i],
					secret, tree->secret_length,
				&tree->nodes[i * tree->node_length], gen_args)) {

			HT_ERROR("failed to calculate leaf hashes\n");
		}
	}

	/* compute hashes on all other levels */
	HT_DEBUG("computing intermediate nodes and root...\n");

	// the leaf level has got full width
	level_width = tree->num_data_blocks;

	// based on the current level, we are calculating the nodes for the next level
	while(level_width > 1) {
		HT_DEBUG("calculating nodes: %i\n", level_width / 2);

		/* set the target for the this level directly behind the
		 * already calculated nodes of the previous level */
		target_index = source_index + (level_width * tree->node_length);

		/* we always handle two elements at once */
		for(i = 0; i < level_width; i += 2) {
			_HT_DEBUG("calling node generator function...\n");

			if(node_gen(&tree->nodes[source_index + (i * tree->node_length)],
					&tree->nodes[source_index + ((i + 1) * tree->node_length)],
					tree->node_length,
					&tree->nodes[target_index + ((i / 2) * tree->node_length)],
					gen_args)) {

				HT_ERROR("failed to calculate hashes of intermediate nodes\n");
			}
			// this means we're calculating the root node
			if(level_width == 2) {
				tree->root = &tree->nodes[target_index + ((i / 2) * tree->node_length)];
			}
		}

		// next level has got half the elements
		level_width = level_width >> 1;

		/* use target index of this level as new source field */
		source_index = target_index;
	}

	out_err:
	return err;
} //]]

/** checks if the hash tree contains further unrevealed data items
 * @param	tree pointer to the tree
 * @return	1 if more elements, else 0
 */
int htree_has_more_data(hash_tree_t *tree) {
//[[
	return tree->data_position < tree->num_data_blocks;
} //]]

/** gets the offset of the next unrevealed data item
 * @param	tree pointer to the tree
 * @return	offset of the data item
 */
int htree_get_next_data_offset(hash_tree_t *tree) {
//[[
	return tree->data_position++;
} //]]

/** gets the elements of the verification branch from a computed tree
 * @param	tree pointer to the hash tree
 * @param	data_index leaf position for which the verification branch is fetched
 * @param	branch_nodes destination buffer for the branch nodes
 * @param	branch_length destination buffer length, returns used space
 * @return	always 0
 */
int htree_get_branch(hash_tree_t *tree, int data_index, unsigned char *branch_nodes, int *branch_length) {
//[[
	int tree_level = 0;
	int level_width = 0;
	int source_index = 0;
	int sibling_offset = 0;
	int err = 0;

	assert(tree != NULL);
	assert(branch_nodes != NULL);
	assert(data_index >= 0);

	// branch includes all elements excluding the root
	*branch_length = tree->depth * tree->node_length;

	HT_DEBUG("tree->depth: %i\n", tree->depth);

	// traverse bottom up
	level_width = tree->num_data_blocks;

	// don't include root
	while (level_width > 1) {
		HT_DEBUG("level_width: %i\n", level_width);

		// for an uneven data_index the previous node is the sibling, else the next
		sibling_offset = data_index & 1 ? -1 : 1;

		// copy branch-node from node-array to buffer
		memcpy(&branch_nodes[tree_level * tree->node_length],
			&tree->nodes[source_index +
			((data_index + sibling_offset) * tree->node_length)],
			tree->node_length);

		// proceed by one level
		source_index += level_width * tree->node_length;
		level_width = level_width >> 1;
		data_index = data_index >> 1;
		tree_level++;
	}

	_HT_HEXDUMP("verification branch: ", branch_nodes, tree->depth * tree->node_length);

	// prevent compiler warning
	goto out_err;

	out_err:
		return err;
} //]]

/** gets the data item at the specified position
 * @param	tree pointer to the hash tree
 * @param	data_index leaf position for which the data item is returned
 * @param	data_length length of the returned data item
 * @return	pointer to the data item, NULL in case of an error
 */
unsigned char* htree_get_data(hash_tree_t *tree, int data_index, int *data_length) {
//[[
	assert(tree != NULL);
	assert(data_index >= 0 && data_index < tree->num_data_blocks);
	assert(data_length != NULL);

	*data_length = tree->max_data_length;

	return &tree->data[data_index * tree->max_data_length];
} //]]

/** gets the secret at the specified position
 * @param	tree pointer to the hash tree
 * @param	data_index leaf position for which the secret is returned
 * @param	secret_length length of the returned secret
 * @return	pointer to the secret, NULL in case of an error
 */
unsigned char* htree_get_secret(hash_tree_t *tree, int data_index, int *secret_length) {
//[[
	assert(tree != NULL);
	assert(data_index >= 0 && data_index < tree->num_data_blocks);
	assert(secret_length != NULL);

	*secret_length = tree->secret_length;

	if(tree->secret_length > 0) {
		return &tree->secrets[data_index * tree->secret_length];
	} else {
		return NULL;
	}
} //]]

/** gets the root node of the hash tree
 * @param	tree pointer to the hash tree
 * @param	root_length length of the returned root element
 * @return	pointer to the root element, NULL in case of an error
 */
unsigned char* htree_get_root(hash_tree_t *tree, int *root_length) {
//[[
	assert(tree != NULL);

	if(tree->root) {
		*root_length = tree->node_length;
	} else {
		*root_length = 0;
	}
	return tree->root;
} //]]

/** checks the data item and an verification branch against the root
 * @param	root pointer to the root
 * @param	root_length length of the root node
 * @param	branch_nodes buffer containing the branch nodes
 * @param	branch_length length of the verification branch
 * @param	verify_data the data item to be verified
 * @param	data_length length of the data item
 * @param	data_index index of the data item in the leaf set
 * @param	secret potentially incorporated secret
 * @param	secret_length length of the secret
 * @param	leaf_gen the leaf generator function pointer
 * @param	node_gen the node generator function pointer
 * @param	gen_args arguments for the generators
 * @param	output_nodes pointer to the buffer where the calculated nodes are stored / or NULL if not used
 * @param	output_leaf point to the buffer where the calculated leaf is stored / or NULL if not used
 * @return	0 if successful, 1 if invalid, -1 in case of an error
 */
int htree_verify_branch(unsigned char *root, int root_length, unsigned char *branch_nodes, uint32_t branch_length, const unsigned char *verify_data,
	int data_length, uint32_t data_index, unsigned char *secret, int secret_length, htree_leaf_gen_t leaf_gen, htree_node_gen_t node_gen,
	htree_gen_args_t *gen_args, unsigned char* output_nodes, unsigned char* output_leaf) {
//[[
	/* space for two nodes to be hashed together */
	unsigned char buffer[2 * root_length];
	int num_nodes = 0;
	int sibling_offset = 0;
	int err = 0, i;

	assert(root != NULL);
	assert(root_length > 0);
	assert(branch_nodes != NULL);
	assert(branch_length > 0);
	assert(verify_data != NULL);
	assert(data_length > 0);

	if(secret_length > 0) {
		assert(secret != NULL);
	}

	num_nodes = branch_length / root_length;

	_HT_DEBUG("num_nodes: %i\n", num_nodes);
	_HT_DEBUG("data_index: %i\n", data_index);
	_HT_DEBUG("data_length: %i\n", data_length);
	_HT_HEXDUMP("verify_data: ", verify_data, data_length);
	_HT_DEBUG("branch_length: %i\n", branch_length);
	_HT_HEXDUMP("verify_data: ", branch_nodes, branch_length);

	// +1 as we have to calculate the leaf too
	for(i = 0; i < num_nodes + 1; i++) {
		HT_DEBUG("round %i\n", i);

		// determines where to put the sibling in the buffer
		sibling_offset = data_index & 1 ? 0 : 1;

		/* in first round we have to calculate the leaf */
		if(i > 0) {
			/* hash previous buffer and overwrite partially */
			if(node_gen(&buffer[0], &buffer[root_length], root_length,
				&buffer[(1 - sibling_offset) * root_length], gen_args)) {

				HT_ERROR("failed to calculate node hash\n");
			}
			if(output_nodes != NULL) {
				memcpy(output_nodes + (i - 1) * root_length, &buffer[(1 - sibling_offset) * root_length], root_length);
			}
		} else {
			/* hash data in order to derive the hash tree leaf */
			if(leaf_gen((const unsigned char *)verify_data, data_length, secret, secret_length,
				&buffer[(1 - sibling_offset) * root_length], gen_args)) {

				HT_ERROR("failed to calculate leaf hash\n");
			}
			if(output_leaf != NULL) {
				memcpy(output_leaf, &buffer[(1 - sibling_offset) * root_length], root_length);
			}
		}



		if(i < num_nodes) {
				// copy i-th branch node to the free slot in the buffer
				memcpy(&buffer[sibling_offset * root_length], &branch_nodes[i * root_length], root_length);
				// proceed to next level
				data_index = data_index >> 1;
		}



		HT_HEXDUMP("buffer slot 1: ", &buffer[0], root_length);
		HT_HEXDUMP("buffer slot 2: ", &buffer[root_length], root_length);
	}

	HT_HEXDUMP("calculated root: ", &buffer[(1 - sibling_offset) * root_length], root_length);
	HT_HEXDUMP("stored root: ", root, root_length);

	// check if the calculated root matches the stored one
	if(!memcmp(&buffer[(1 - sibling_offset) * root_length], root, root_length)) {
		HT_DEBUG("branch successfully verified\n");
	} else {
		HT_DEBUG("branch invalid\n");
		err = 1;
	}

	out_err:
	return err;
} //]]

/** generates a leaf node from a given data item
 * @param	data data item to be hashed
 * @param	data_length length of the data item
 * @param	secret potentially incorporated secret
 * @param	secret_length length of the secret
 * @param	dst_buffer buffer for the generated leaf node
 * @param	gen_args arguments for the generator
 * @return	always 0
 */
int htree_leaf_generator(unsigned char *data, int data_length, unsigned char *secret, int secret_length, unsigned char *dst_buffer, htree_gen_args_t *gen_args) {
//[[
	int err = 0;
	unsigned char buffer[data_length + secret_length];
	unsigned char *hash_data = NULL;
	int hash_data_length = 0;

	if(secret && secret_length > 0) {
		memcpy(&buffer[0], data, data_length);
		memcpy(&buffer[data_length], secret, secret_length);

		hash_data = buffer;
		hash_data_length = data_length + secret_length;

	} else {
		hash_data = data;
		hash_data_length = data_length;
	}

	if(!SHA1(hash_data, hash_data_length, dst_buffer)) {
		HT_ERROR("failed to calculate hash\n");
	}
	out_err:
		return err;
} //]]

/** generates an intermediate node from two hash tree nodes
 * @param	left_node the left node to be hashed
 * @param	right_node the right node to be hashed
 * @param	node_length length of each node
 * @param	dst_buffer buffer for the generated intermediate node
 * @param	gen_args arguments for the generator
 * @return	0 on success, -1 in case of an error
 */
int htree_node_generator(unsigned char *left_node, unsigned char *right_node, int node_length, unsigned char *dst_buffer, htree_gen_args_t *gen_args) {
//[[
	int err = 0;

	/* the calling function has to ensure that left and right node are in
	 * subsequent memory blocks */
	if(!SHA1(left_node, 2 * node_length, dst_buffer)) {
		HT_ERROR("failed to calculate hash\n");
	}
	out_err:
		return err;
} //]]

/** Print all leaves of a tree
 * @param	tree		Pointer to the MT
 */
void htree_print_data(hash_tree_t *tree) {
//[[
	int i;

	assert(tree != NULL);

	HT_DEBUG("printing data blocks...\n");

	for(i = 0; i < tree->num_data_blocks; i++) {
		HT_HEXDUMP("data block: ", &tree->data[i * tree->max_data_length], tree->max_data_length);
	}
} //]]

/** Print all nodes of a tree
 * @param	tree		Pointer to the MT
 */
void htree_print_nodes(hash_tree_t *tree) {
//[[
	int level_width = 0;
	int target_index = 0;
	int source_index = 0;
	int i = 0;

	assert(tree != NULL);

	level_width = tree->num_data_blocks;

	HT_DEBUG("printing hash tree nodes...\n");

	while (level_width > 0) {
		i++;
		HT_DEBUG("printing level %i:\n", i);

		target_index = source_index + (level_width * tree->node_length);

		for(i = 0; i < level_width; i++) {
			HT_HEXDUMP("node: ", &tree->nodes[source_index + (i * tree->node_length)], tree->node_length);
		}

		source_index = target_index;
		level_width = level_width >> 1;
	}
} //]]

/** calculates the logarithm for a given base
 * @param	base the base of the logarithm
 * @param	value value for which the log should be computed
 * return	the log
 */
double log_x(int base, double value) {
//[[
	return log(value) / log(base);
} //]]
