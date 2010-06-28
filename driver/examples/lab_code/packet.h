// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 *  @file 	packet.h
 *  @brief 	ALPHA packet definitions
 */

#ifndef __PACKET_H
#define __PACKET_H

#include "alpha.h"
#include <inttypes.h>

// Address verification packets
#define PACKET_CONNECT		1
#define PACKET_RETURN_CONNECT	2

// Handshake packets
#define	PACKET_SYN		3
#define PACKET_ACK		4
#define PACKET_ACKACK		5

// Signature scheme packets (aka payload packets)
#define PACKET_S1		6
#define PACKET_A1		7
#define PACKET_S2		8

#define PACKET_ALPHA_Z		9

// Control messages for new alpha associations
#define PACKET_NEW_ASS		9
#define PACKET_NEW_ASS_ACK	10
#define PACKET_ASS_DIE 		11

#define PACKET_LOWEST		1
#define PACKET_HIGHEST		9

// size of the largest header of any alpha packet in bytes
// update this everytime you enlarge any packets!
#define LARGEST_ALPHA_HEADER	(2 * HASHSIZE + sizeof(alpha_packet_s2_m_t))

// General packets
//[[
/** @brief: most basic packet type. every other packet can be casted to this one */
typedef struct alpha_packet {
	uint8_t		type;
	uint8_t		association_id;
} __attribute__((__packed__)) alpha_packet_t;
//]]

// Handshake packets
//[[
/** @brief: first handshake packet (SYN) */
typedef struct alpha_packet_syn {
	uint8_t		type;
	uint8_t		association_id;
	uint32_t	challenge;
	unsigned char	addr_hash[HASHSIZE];
	unsigned char	sign_anchor[HASHSIZE];		// these are unprotected, yes
	unsigned char	ack_anchor[HASHSIZE];
} __attribute__((__packed__)) alpha_packet_syn_t;

/** @brief: second handshake packet (ACK) */
typedef struct alpha_packet_ack {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	challenge_response[SIGSIZE];	// signature of the received + sent anchors + challenge
	unsigned char	sign_anchor[HASHSIZE];
	unsigned char	ack_anchor[HASHSIZE];
} __attribute__((__packed__)) alpha_packet_ack_t;

/** @brief: third handshake packet (ACKACK) */
typedef struct alpha_packet_ackack {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	ack[HASHSIZE];
} __attribute__((__packed__)) alpha_packet_ackack_t;

typedef struct alpha_packet_connect {
	uint8_t		type;
	uint8_t		association_id;
} __attribute__((__packed__)) alpha_packet_connect_t;

typedef struct alpha_packet_return_connect {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	addr_hash[HASHSIZE];
} __attribute__((__packed__)) alpha_packet_return_connect_t;
//]]

// Payload packets
//[[
/** @brief: first signature scheme packet for payload (aka S1) */
typedef struct alpha_packet_s1{
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	anchor[HASHSIZE];
	uint16_t 	signatures_count;
	//presignature is in the payload
} __attribute__((__packed__)) alpha_packet_s1_t;

//! S1 packet for alpha m
typedef struct alpha_packet_s1_m {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	anchor[HASHSIZE];
	uint16_t 	signatures_count;
	//! Number of nodes per packet
	uint8_t sec_mode;
	//presignature is in the payload
} __attribute__((__packed__)) alpha_packet_s1_m_t;

/** @brief: second signature scheme packet for payload (aka A1) */
typedef struct alpha_packet_a1 {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	anchor[HASHSIZE];
	unsigned char	return_anchor[HASHSIZE];
} __attribute__((__packed__)) alpha_packet_a1_t;

/** @brief: third signature scheme packet for payload (aka S2) */
typedef struct alpha_packet_s2 {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	anchor[HASHSIZE];
	/* payload follows: starts at base+sizeof(alpha_packet_s2) */
} __attribute__((__packed__)) alpha_packet_s2_t;

//! S2 packet for alpha m
typedef struct alpha_packet_s2_m {
	uint8_t		type;
	uint8_t		association_id;
	unsigned char	anchor[HASHSIZE];
	//! The index of the message in the hashtree
	uint16_t	data_index;
	/* payload follows (includes branches): starts at base+sizeof(alpha_packet_s2_m_t) */
} __attribute__((__packed__)) alpha_packet_s2_m_t;

//]]

// Packet to bootstrap new associations
//[[

//! Control packet
/*!
	Control packets are encapsulated in regular Alpha packets.
	Currently they are used to agree on new association (and to bootstrap them)
 */
typedef struct alpha_control_packet {
	//! Type of the control packet
	uint8_t type;
} __attribute__((__packed__)) alpha_control_packet_t;

//! Packet to bootstrap new associations
typedef struct alpha_packet_new_association {
	//! Packet type, always PACKET_NEW_ASS
	uint8_t		type;
	//! the id of the association on which this packet comes in
	uint8_t		association_id;
	//! Hash element of the association which sends this packet (usually the default association)
	unsigned char	anchor[HASHSIZE];
	//! Number of associations which are bootstrapped by this packet
	uint8_t 	num_associations;
	// ([ass id][mode][sign anchor][ack anchor])* follows
} __attribute__((__packed__)) alpha_packet_new_ass_t;

//! Packet to ack that a new association has been created
typedef struct alpha_packet_new_ass_ack {
	//! the type of the packet, always PACKET_NEW_ASS_ACK
	uint8_t		type;
	//! the id of the association on which this packet comes in
	uint8_t		association_id;
	//! the anchor of the ACK hash chain (from the default association)
	unsigned char	anchor[HASHSIZE];
	//! The previously received anchor (from the default association)
	unsigned char	ret_anchor[HASHSIZE];
	//! Number of associations which are bootstrapped by this packet
	uint8_t		num_associations;
	// ([ass id][mode][sign anchor][ack anchor])* follows
} __attribute__((__packed__)) alpha_packet_new_ass_ack_t;

//! Tells the association to kill a given assoication
/*!
	The association, on which this packet comes in kills the
	given associations.
 */
typedef struct alpha_packet_kill_ass{
	//! Type of the control packet
	uint8_t type;
	//! (ass_id)* follows
} __attribute__((__packed__)) alpha_packet_kill_ass_t;

//]]

#endif // __PACKET_H
