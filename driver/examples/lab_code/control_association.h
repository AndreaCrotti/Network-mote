// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef CONTROL_ASS_H_
#define CONTROL_ASS_H_

#include <sys/socket.h>
#include <stdlib.h>

#include "alpha.h"
#include "packet.h"
#include "association.h"
#include "alpha_n.h"

bool ca_is_ctrl_packet(const alpha_packet_s2_t*);

ap_err_code ca_decode_ass(const size_t, const unsigned char*, const size_t,
	unsigned char*, unsigned char*, const size_t);

ap_err_code ca_encode_ass(const size_t, unsigned char*, const size_t, const uint8_t,
	const uint8_t, const unsigned char*, const unsigned char*, const size_t hashsize);

ap_err_code ca_decode_ass_metadata(const size_t, const unsigned char*, const size_t,
	uint8_t* const, uint8_t* const);

ap_err_code ca_hdl_ctrl_packet(const unsigned int, const config_t*, const unsigned char*, const size_t);

ap_err_code ca_req_ctrl_ass(const unsigned int, association_t*);

ap_err_code ca_request_new_asss(const unsigned int, const config_t*, const size_t, const size_t, const size_t, const size_t);

ap_err_code ca_kill_ass(const unsigned int, const config_t*, const unsigned int*, const unsigned int);

#endif /*CONTROL_ASS_H_*/
