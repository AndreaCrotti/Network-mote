// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __ASS_RELAY_ALPHA_Z_H__
#define __ASS_RELAY_ALPHA_Z_H__

#include "association_relay.h"

//! A Alpha Z association
/*!
 * @note SYNC with alpha_ass_relay
 */
struct alpha_z_ass_relay {
	//! unique identifier for ALL association relays (of any type)
	unsigned int id;
	//! the mode (for this its always ALPHA_Z)
	unsigned int mode;
	//! Sign anchors (not used in Alpha Z, but required by association_relay)
	struct ring_buffer* sign_anchors;
	//! ack anchors (not used in Alpha Z, but required by association_relay)
	struct ring_buffer* ack_anchors;
};

void association_relay_free_alpha_z_ass(struct alpha_z_ass_relay* ass);
void association_relay_init_alpha_z_ass(struct alpha_z_ass_relay* ass,
	const unsigned int id, const alpha_mode_t mode);

#endif // __ASS_RELAY_ALPHA_Z_H__
