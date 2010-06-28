// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include "cache_tree.h"

#include "extended_math.h"
#include "digest.h"
#include "tools.h"
#include "xmalloc.h"
#include "hashtree.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>

//! Gets the number of sent signatures per packet depending on the security mode
/*!
 * @param[in] sec_mode the security mode which should be checked
 * @return number of signatures per packet
 */
unsigned int cache_tree_get_number_of_sent_signatures(const cache_tree_security_mode_t sec_mode) {
//[[
	switch(sec_mode) {
		case CACHE_TREE_ONE_SIGNATURE_PER_PACKET :
			return 1;
		case CACHE_TREE_TWO_SIGNATURES_PER_PACKET:
			return 2;
	}
	return 0;
} //]]

//! Gets the position of a given level for a given index
/*!
 * Returns the position of the node on the given level which is needed to calculate
 * the root for a given data_index (= leaf index). Note that you always need the
 * opposing node (i.e. the sibling node) to calculate the root. You need on each
 * level the sibling node. The position returned is the i-th node in the l-th level
 * which is needed to calculate the root. (l is given in this function, i is returned).
 *
 * @param[in] data_index the leaf index
 * @param[in] level the level on which the position should be returned
 * @return the positon/index of the node in the given level which is needed to calculate the root
 */
unsigned int cache_tree_get_position(const unsigned int data_index, const unsigned int level) {
//[[
	unsigned int k;
	unsigned int pos = data_index;
	for(k = 0; k < level; ++k){
		pos /= 2;
		// pos % 2 != 0
		if( (pos & 1) != 0 ) {
			//pos uneven
			pos--;
		} else {
			//pos even
			pos++;
		}
	}
	return pos;
} //]]

//! Counts the 1-bits in a given integer
/*!
 * Calculates the number of 1-bits in a given integer.
 *
 * @param[in] value the integer that is checked
 * @return number of positive, i.e. 1-bits
 */
static unsigned int cache_tree_count_bits(const unsigned int value) {
//[[
	unsigned int i = 0;
	unsigned int c = 0;
	for(i = 0; i < (sizeof(unsigned int) * 8); ++i) {
		if( (value & (1 << i)) != 0 ) {
			c++;
		}
	}
	return c;
} //]]

//! Checks whether a node is a supporter for a different node
/*!
 * When starting with a new subtree, more than one node is usually required for verification.
 * When only transmitting one or two nodes per packet this might not be enough. Therefore
 * other nodes need to transmit nodes that are required for further verification. We call the
 * nodes, that cannot include enough packets to get verified helped_leaf and the one that
 * is offering help, i.e. by transmitting a node that is not need for their own verification, the helper_index.
 *
 * @param[in] helper_index the packet offering help
 * @param[in] helped_leaf the packet index that needs help
 * @param[in] leaf_count the number of nodes in the tree
 *
 * @returns helper_index if the helper_index is really a helper for helped_leaf, otherwise 0
 */
static unsigned int cache_tree_is_helper(const unsigned int helper_index, const unsigned int helped_leaf, const unsigned int leaf_count) {
//[[
	assert(helped_leaf > helper_index);
	assert(helped_leaf < leaf_count);

	unsigned int i;
	unsigned int c = 0;
	unsigned int x = (helper_index - 1) ^ helper_index;

	if(cache_tree_count_bits(x) > 2) {
		return false;
	}

	x = (helped_leaf - 1) ^ (helped_leaf);
	c = cache_tree_count_bits(x);
	unsigned int dist = c - 2;
	unsigned int cur_leaf;
	unsigned int j;
	bool is_helper;
	i = 1;
	for(j = 1; i < dist; ++j) {
		cur_leaf = helped_leaf - (2 * j);
		x = (cur_leaf - 1) ^ cur_leaf;
		if(cache_tree_count_bits(x) <= 2) {
			// check if there's a different node which occupies the helping node
			unsigned int k = 0;
			is_helper = true;
			for(k = helped_leaf - 2; k > cur_leaf ; k-=2) {
				if(cache_tree_is_helper(cur_leaf, k, leaf_count)) {
					is_helper = false;
					break;
				}
			}
			if(!is_helper) {
				continue;
			}
			//cur leaf is helper
			if(cur_leaf == helper_index) {
				return i;
			}
			i++;
		}
	}
	return 0;
} //]]

//! Gets the node which should be pre-sent in the given packet
/*!
 * This function checks which node needs to be transmitted for a given data index and
 * leaf count.
 *
 * @param[in] data_index the index of the packet
 * @param[in] leaf_count the total number of packets in the tree
 * @param[out] out_index index of the node/hash to be sent (in the tree)
 * @param[out] out_level level of the node/hash to be sent (in the tree)
 *
 * @note output parameters need to be preallocated
 */
void cache_tree_get_pre_send_node(const unsigned int data_index, const unsigned int leaf_count, unsigned int *out_index, unsigned int *out_level) {
//[[
	// we only perform forward sending on uneven packets (on even packets we need to transmit signatures for the next uneven packet)
	assert(data_index % 2 == 1);
	// leaf_count = 2^x for a integer x
	assert(exp_2(log_2(leaf_count)) == leaf_count);
	assert(out_index != NULL);
	assert(out_level != NULL);

	if(data_index == leaf_count - 1) {
		*out_level = 0;
		return;
	}

	if(leaf_count <= 4) {
		*out_level = 0;
		return;
	}

	// Calculates the previous iteration before end in a binary search
	// returns deadline = previous iteration
	unsigned int n = (leaf_count) / 2;

	unsigned int step = n/2;

	const unsigned int search_index = data_index + 1;
	unsigned int level = log_2(leaf_count) - 2;
	// if this node helps a different node to fulfill a deadline
	bool is_helper = false;
	unsigned int helper_dist = 0;

	while(search_index != n){
		if(search_index < n) {
			// check if node is a helper
			helper_dist = cache_tree_is_helper(search_index, n, leaf_count);
			if(helper_dist != 0) {
				level = helper_dist + 1;
				is_helper = true;
				break;
			}
			// search on the left side
			n -= step;
		} else {
			// >
			n += step;
		}
		step = step / 2;
		if(level > 0) {
			level--;
		}
	}
	if(!is_helper && level > 1) {
		level = 1;
	}
	*out_level = level;
	if(level == 0) {
		return;
	}

	*out_index = cache_tree_get_position(n, level);
} //]]

//! Get the nodes that are transmitted for a given packet index
/*!
 * This uses gets the nodes that are required to sent in a given packet.
 *
 * @param[in] tree the hash tree containing the nodes that are to sent
 * @param[in] leaf_count the number of leafs in the hash tree (we do not use the internal structure of htree to get these, so please supply the value yourself)
 * @param[in] data_index the packet index
 * @param[in] mode the security mode, i.e. how many nodes are included in each packet
 * @param[in] hashsize the output size of the hash function, i.e. size of each node
 * @param[out] pointer to a buffer in which the nodes (if there are more than one) are copied to
 */
void cache_tree_get_nodes(const hash_tree_t* tree, const unsigned int leaf_count, const unsigned int data_index,
	const cache_tree_security_mode_t mode, const unsigned int hashsize, unsigned char** out) {
//[[
	assert(tree != NULL);
	assert(tree->num_data_blocks > 0);
	assert(data_index < (unsigned int)tree->num_data_blocks);
	assert(hashsize > 0);
	assert(out != NULL);

	unsigned int tree_index, level;
	if( (data_index & 0x1) == 0) {
		//even
		//\TODO sent next leaf and node of previous tree
		memcpy(*out, &tree->nodes[(data_index+1)*hashsize], hashsize);

		if(mode == CACHE_TREE_ONE_SIGNATURE_PER_PACKET) {
			return;
		}


		if(data_index == 0) {
			return;
		}
		// node of the previous tree =>
		//  we need to get up 'level' levels up and take then the sibling
		//  how much we need to go up depends on the previous node
		cache_tree_get_pre_send_node(data_index - 1, leaf_count, &tree_index, &level);
		unsigned int i;
		tree_index = data_index;
		for(i = 0; i < level + 1; ++i) {
			tree_index /= 2;
		}
		// get sibling
		if( (tree_index & 1) == 0 ) {
			// even
			tree_index += 1;
		} else {
			tree_index -= 1;
		}
		memcpy(*out + hashsize, &tree->nodes[hashsize * htree_get_node_index(level + 1, leaf_count, tree_index)], hashsize);

	} else {
		// odd
		cache_tree_get_pre_send_node(data_index, leaf_count, &tree_index, &level);
		if(level > 0) {
			switch(mode) {
				case CACHE_TREE_ONE_SIGNATURE_PER_PACKET:
					// sent a forward node
					memcpy(*out,
						&tree->nodes[hashsize * (htree_get_node_index(level, leaf_count, tree_index))],
						hashsize);
					break;
				case CACHE_TREE_TWO_SIGNATURES_PER_PACKET:
					// send a forward node and its silbling
					if( (tree_index & 1) == 0) {
						// even
						memcpy(*out,
							&tree->nodes[hashsize * (htree_get_node_index(level, leaf_count, tree_index))],
							2*hashsize);
					} else {
						// odd
						// first the sibling, then the node
						memcpy(*out,
							&tree->nodes[hashsize * (htree_get_node_index(level, leaf_count, tree_index) - 1)],
							hashsize);
						memcpy(*out + hashsize,
							&tree->nodes[hashsize * (htree_get_node_index(level, leaf_count, tree_index))],
							hashsize);
					}
					break;
			}
		} else {
			switch(mode) {
				case CACHE_TREE_ONE_SIGNATURE_PER_PACKET:
					// we sent the previous leaf
					memcpy(*out, &tree->nodes[hashsize * (data_index - 1)],
						hashsize);
					break;
				case CACHE_TREE_TWO_SIGNATURES_PER_PACKET:
					// we sent all neccessary
					// level 0:
					memcpy(*out, &tree->nodes[hashsize * (data_index - 1)],
						hashsize);
					// level 1:
					unsigned int parent_index = (data_index / 2);
					if( (parent_index & 1) == 0 ) {
						// even
						memcpy(*out + hashsize, &tree->nodes[hashsize * htree_get_node_index(1, leaf_count, parent_index + 1)], hashsize);
					} else {
						memcpy(*out + hashsize, &tree->nodes[hashsize * htree_get_node_index(1, leaf_count, parent_index - 1)], hashsize);
					}
					break;
			}
		}
	}

} //]]

//! Allocated space to store nodes
/*!
 * See alpha_m_ass_store_node() for more detailed documentation.
 * This function sets the allocated space to zero before returning.
 *
 * @param[in] prev_buf NULL or pointer to some previous allocated space
 * @param[in] leaf_count the size of the hash tree, whose nodes should be stored
 * @param[in] hashsize the size of a single node
 * @return pointer to the new alloced space or NULL in case of error
 */
unsigned char* cache_tree_reallocate_node_buffer(unsigned char* prev_buf, const unsigned int leaf_count, const unsigned int hashsize) {
//[[
	//alloc buffer
	const unsigned int buffer_size = 2*(log_2(leaf_count)) * hashsize;
	unsigned char* buf = xrealloc(prev_buf, buffer_size);
	memset(buf, 0, buffer_size);
	return buf;
} //]]

//! Stores a node in the node buffer
/*!
 * Stores(copies) a node in a preallocated node buffer. Use cache_tree_reallocate_node_buffer() to get a buffer
 * of the required size. The level parameter should range from [1,(log2(n)-1)] where n is the number
 * leafs in the tree. The level of the leaf nodes is 0 and the level of the root node is log2(n).
 * This shows that neither leaf nor root nodes are stored in the buffer. The position of the node
 * describes which node in which given level should be addressed. There are on level 0 for example
 * leaf_count many leaf nodes(i.e. leaf_count many positions). On level 1 there are half the
 * number leaf_count positions and so on.
 *
 * @note Be sure to use the correct level! 0 is starting at the leafs, log2(n) at the root of the tree!
 *
 * @param[in,out] node_buf the buffer where nodes should be stored
 * @param[in] node the actual node which should be COPIED into the buffer
 * @param[in] position the position of the node in the given level (0 or 1)
 * @param[in] level the level of the node in the given tree
 * @param[in] leaf_count the size of the tree
 * @param[in] hashsize the size of a node
 *
 * @see cache_tree_reallocate_node_buffer(), cache_tree_get_node()
 */
void cache_tree_store_node(unsigned char* node_buf, const unsigned char* node, const unsigned int position,
	const unsigned int level, const unsigned int leaf_count, const unsigned int hashsize){
//[[
	//level should NOT be root level! (root is calculated and then compared with a stored root)
	assert(level < log_2(leaf_count));
	//leaf count is of form 2^x!
	assert(leaf_count == exp_2(log_2(leaf_count)));

	assert(position == 0 || position == 1);
	memcpy(&(node_buf[(2*level + position) * hashsize]), node, hashsize);
} //]]

//! Retreives a node from a given buffer
/*!
 * For further more detailed documentation see cache_tree_store_node()
 * @param[in] node_buf the buffer where the nodes are stored
 * @param[out] node preallocated space where the node is copied to
 * @param[in] position the position of the node in the given level (0 or 1)
 * @param[in] level the level of the node in the tree
 * @param[in] leaf_count the size of the tree
 * @param[in] hashsize the size of a node
 *
 * @see cache_tree_reallocate_node_buffer(), cache_tree_store_node()
 */
void cache_tree_get_node(const unsigned char* node_buf, unsigned char* node, const unsigned int position,
	const unsigned int level, const unsigned int leaf_count, const unsigned int hashsize) {
//[[
	//level should NOT be root level! (root is calculated and then compared with a stored root)
	assert(level < log_2(leaf_count));
	//leaf count is of form 2^x!
	assert(leaf_count == exp_2(log_2(leaf_count)));
	memcpy(node, &node_buf[(2 * level + position) * hashsize], hashsize);
} //]]

//! Updates the node cache for alpha m
/*!
 * @param[in,out] node_buf the buffer on which the nodes are updated
 * @param[in] data_index the index of the incoming packet
 * @param[in] leaf_count the number of leafs in the hash tree
 * @param[in] node the node which is included in the received packet
 * @param[in] calc_nodes the nodes which have been calculated and verified by the hash tree
 * @param[in] mode the mode of security
 * @param[in] hashsize the size of the nodes
 *
 * @see htree_verify_branch()
 */
void cache_tree_update_buffer(unsigned char* node_buf, const unsigned int data_index, const unsigned int leaf_count,
	const unsigned char* nodes, const unsigned char* calc_nodes, const cache_tree_security_mode_t mode,
	const unsigned int hashsize) {
//[[
	assert(data_index < leaf_count);
	assert(nodes != NULL);
	assert(hashsize > 0);

	if(node_buf == NULL) {
		return;
	}

	// first we store the calculated nodes
	unsigned int idx = data_index;
	unsigned int i;
	for(i = 0; i < log_2(leaf_count); ++i) {
		// we only store the nodes on the level where the node is even
		if( (idx & 1) == 0 ){
			// even
			cache_tree_store_node(node_buf, calc_nodes + (i*hashsize), 0, i, leaf_count, hashsize);
		}
		idx = idx / 2;
	}

	// secondly we store the pre-sent node
	if((data_index & 1) != 0) {
		// odd
		// node is a present node in the tree
		unsigned int level, tree_index;
		cache_tree_get_pre_send_node(data_index, leaf_count, &tree_index, &level);
		if(level > 0) {
			if(mode == CACHE_TREE_ONE_SIGNATURE_PER_PACKET || (tree_index & 1) == 0) {
				// even
				cache_tree_store_node(node_buf, nodes, 1, level, leaf_count, hashsize);
			} else {
				// odd (the node is at the second slot
				cache_tree_store_node(node_buf, nodes + hashsize, 1, level, leaf_count, hashsize);
			}

		}
	}
} //]]

//! Checks whether a cache entry may be updated
/*!
 * This only works with CACHE_TREE_TWO_SIGNATURES_PER_PACKET and uneven packets!
 * @param[in] node_buf the buffer for storing nodes (the 'cache')
 * @param[in] left_node the first node of the two signatures
 * @param[in] right_node the second node of the two signatures
 * @param[in] data_index index of the received packet
 * @param[in] leaf_count size of the tree
 * @param[in] node_gen pointer to node generator function
 * @param[in] gen_args arguments for the node generator function
 * @param[in] hashsize size of a node
 * @return true if it is safe to update the cache
 *
 * @see cache_tree_get_pre_send_node(), cache_tree_update_buffer()
 */
bool cache_tree_verify_new_cache_entry(const unsigned char* node_buf, const unsigned char* left_node, const unsigned char* right_node,
	const unsigned int data_index, 	const unsigned int leaf_count, htree_node_gen_t node_gen, htree_gen_args_t* gen_args, const unsigned int hashsize) {
//[[
	assert(left_node != NULL);
	assert(right_node != NULL);

	if(node_buf == NULL) {
		// no buffering, therefore it does not make a difference if we allow/disallow buffering
		return false;
	}

	// we can only verify odd packets
	if( (data_index & 1) == 0 ) {
		return false;
	}

	unsigned char parent[hashsize];
	unsigned char expected_parent[hashsize];
	unsigned int forward_node_level, forward_node_index;

	cache_tree_get_pre_send_node(data_index, leaf_count, &forward_node_index, &forward_node_level);

	// we can only verify nodes which do have a forwarding node
	if( forward_node_level == 0 ) {
		return false;
	}

	unsigned int position = (((forward_node_index / 2) & 1) == 0) ? 0 : 1;


	cache_tree_get_node(node_buf, expected_parent, position, forward_node_level, leaf_count, hashsize);
	node_gen(left_node, right_node, hashsize, parent, gen_args);
	return (memcmp(parent, expected_parent, hashsize) == 0);
} //]]

//! Creates a buffer containing the branches which are needed for verification
/*!
 * @param[in] node_buf the buffer containg the nodes (aka 'cache')
 * @param[in] leaf the leaf (only needed for even nodes)
 * @param[out] out pointer to a buffer where the branches are stored (is log_2(leaf_count) * hashsize)
 * @param[in] data_index the leaf index
 * @param[in] leaf_count number of leafs in the tree
 * @param[in] mode the mode of security
 * @param[in] hashsize size of the hash function used in the tree
 *
 * @see cache_tree_rellocate_node_buffer
 */
void cache_tree_build_verification_buffer(const unsigned char* node_buf, const unsigned char* packet_signatures,
	unsigned char** out, const unsigned int data_index, const unsigned int leaf_count, const cache_tree_security_mode_t sec_mode,
	const unsigned int hashsize) {
//[[

	// node_buf can be zero if we do not buffer leaves and the tree is of size 2!
	assert(packet_signatures != NULL);
	assert(out != NULL);
	assert(*out != NULL);
	assert(data_index < leaf_count);

	unsigned char node[hashsize];

	// copy nodes
	unsigned int k;
	unsigned int parent_index = data_index;
	const unsigned int depth = log_2(leaf_count);

	if(leaf_count == 2) {
		memcpy(*out, packet_signatures, hashsize);
		return;
	}

	for(k = 1; k < depth; ++k) {
		parent_index /= 2;
		// parent_index % 2 != 0
		if((parent_index & 1) != 0) {
			// odd!
			cache_tree_get_node(node_buf, node, 0, k, leaf_count, hashsize);
		} else {
			// even
			cache_tree_get_node(node_buf, node, 1, k, leaf_count, hashsize);
		}
		memcpy((*out) + (hashsize * k), node, hashsize);
	}

	// insert leaf
	if( (data_index & 1) == 0) {
		// data_index % 2 == 0 => even
		// we got a leaf!
		memcpy(*out, packet_signatures, hashsize);
		/*if(sec_mode == CACHE_TREE_TWO_SIGNATURES_PER_PACKET) {
			//we have also received the node from a different subtree (maybe)
			unsigned int index, level;
			cache_tree_get_pre_send_node(data_index - 1, leaf_count, &index, &level);
			if(level > 0) {
				memcpy(*out + (level * hashsize), packet_signatures + hashsize, hashsize);
			}
		}*/
	} else {
		unsigned int level, tree_index;
		cache_tree_get_pre_send_node(data_index, leaf_count, &tree_index, &level);
		if(level == 0) {
			// we have received the leaf
			memcpy(*out, packet_signatures, hashsize);
			if(sec_mode == CACHE_TREE_TWO_SIGNATURES_PER_PACKET) {
				// we have also received the node on the next level
				memcpy(*out + hashsize, packet_signatures + hashsize, hashsize);
			}
		} else {
			cache_tree_get_node(node_buf, node, 0, 0, leaf_count, hashsize);
			memcpy(*out, node, hashsize);
			// CACHE_TREE_TWO_SIGNATURES_PER_PACKET
			// -> we received the sibling but cannot use it to build a better verification buffer
		}
	}

} //]]
