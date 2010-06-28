// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#include "association_relay_alpha_z.h"
#include "../xmalloc.h"

#include <assert.h>

//! Initializes a given Alpha Z association
/*!
 * Initializes a Alpha Z association
 * 
 * @param[in,out] ass the association to be initalized (prealloced)
 * @param[in] id the id of the association (should be unique ranging from 0-255)
 * @param[in] mode always ALPHA_Z
 */
void association_relay_init_alpha_z_ass(struct alpha_z_ass_relay* ass,
	const unsigned int id, const alpha_mode_t mode) {
//[[
	assert(ass != NULL);
	ass->id = id;
	ass->mode = mode;
	ass->sign_anchors = ring_buffer_new(STORE_SIGN_ANCHORS, HASHSIZE);
	ass->ack_anchors = ring_buffer_new(STORE_ACK_ANCHORS, HASHSIZE);
} //]]

//! Frees a given ALpha Z Association
/*!
 * The association is freed if a not null pointer is passed. 
 * If the pointer is a null pointer no operation is performed. 
 * 	
 * @note This function should be called by association_relay_free() only
 * @param[in] ass the association to be freed
 */
void association_relay_free_alpha_z_ass(struct alpha_z_ass_relay* ass) {
//[[
	if(ass == NULL) {
		return;
	}

	xfree(ass);
} //]]
