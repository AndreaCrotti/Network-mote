// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef __DEFINES_H_
#define __DEFINES_H_

#ifndef MACOSX
 #define HAVE_ON_EXIT
#endif

//! Runs the Alpha Zero mode
/*!
 * The Alpha zero mode is a mode, which just
 * transmits packets with a random Alpha
 * header containing any information. On the
 * client side all packets will be accepted.
 * This can be set for measurement issues,
 * to compare how much overhead is caused
 * by verification.
 */
//#define RUN_ALPHA_ZERO

#ifdef REVISION
#define VERSION "0.2-ws09-"REVISION
#else
#define VERSION "0.2-ws09"
#endif

// Name of the configuration file
#define CONFIG "alpha.conf"

// Path to the system-wide configuration file
#define CONFIGPATH "/etc"

// Path to default alpha logfile (for daemonized mode)
#define DEFAULT_LOGFILE "alpha.log"

// Path to the socket file, which the alpha control tool uses to talk to the alpha daemon
#define SOCKETFILE "alpha.sock"

// Debug verbosity
#ifdef DEBUG
//#define DEBUG_HANDSHAKE
//#define DEBUG_SIGSCHEME
//#define DEBUG_DIGEST
//#define DEBUG_TUN
//#define DEBUG_NET
//#define DEBUG_HASHCHAIN
//#define DEBUG_HASHTREE
//#define DEBUG_PUBKEY
#endif

// Defines the number of a characters of a digests string represenation
// should be actually printed in log files (set to 2*HASHSIZE for the
// whole thing)
#define DIGEST_PRINTLEN 10

// The length in seconds of the slot in which a connect/return_connect is valid
#define TIMESLOT_SIZE	10

// The UDP_MAX_PACKET_SIZE is the size of the biggest possible UDP packet we
// assume to be able to send without fragmentation; this is the default
// MTU (1500 on most systems) minus the maximum size of the ip header (60 bytes)
// minus the maximum size of the udp header
// The TUN_MAX_PACKET_SIZE is the biggest possible payload which we can
// encapsulate, that is the udp packet minus the size of the biggest
// alpha header we need. This will be set as MTU for the tun interface
// TODO: All this should be implemented somehow using Path MTU Discovery, since
// the actual MTU can vary for different endpoints; Problem: we use one tun
// interface for multiple endpoints... Hmmm.. lets think about that later
#define BACKEND_MTU 1500
#define UDP_MAX_PACKET_SIZE (BACKEND_MTU - 60 - 8)
#define TUN_MAX_PACKET_SIZE (UDP_MAX_PACKET_SIZE - LARGEST_ALPHA_HEADER)
#define MAX_PRESIG_COUNT ((int)((UDP_MAX_PACKET_SIZE - sizeof(alpha_packet_s1_t)) / HASHSIZE))

// The handshake timeout in seconds; If after this number of seconds,
// no ACK for a sent SYN was received, SYN is sent again, etc.
#define HANDSHAKE_TIMEOUT	rand()%2 + 1
#define S1_TIMEOUT		HANDSHAKE_TIMEOUT

// Default port of the alpha daemon
#define PORT 1234

// Byte-count of the digest we use for signing/ack-ing and MAC (SHA1: 20 bytes, MD5: 16 bytes)
#define HASHSIZE 20
// Byte-count of pubkey-signature (DSA_size right now)
#define SIGSIZE 48
// Number of pre-hashed elements to store for each client and chain (HASHCHAIN_LENGTH _HAS_TO_BE_ a multiple of this!)
#define CHAIN_STORE_SIZE 1000

typedef enum {
	AP_ERR_SUCCESS 			=  1,  // It worked!
	AP_ERR_NOMEM   			= -1,  // Not enough memory
	AP_ERR_SOCKET  			= -3,  // Socket error
	AP_ERR_INVALID_MODE 		= -4,  // not definied Alpha Mode is running
	AP_ERR_STATE			= -5,  // State error. Unexpected input
	AP_ERR_INVALID_ASS	= -6,
	AP_ERR_INVALID_CONTROL_TYPE = -7, //invalid control packet type
} ap_err_code;

/* Debug levels */
/** Only print error messages */
#define AP_MSG_LVL_ERROR		0

/** Print errors and warnings */
#define AP_MSG_LVL_WARN			10

/** Default level for ALPHA as daemon (less chatty) */
#define AP_MSG_LVL_DAEMON		20

/** Default level for ALPHA as application */
#define AP_MSG_LVL_DEFAULT		30

/** Print debug messages */
#define AP_MSG_LVL_DEBUG		40

/** Print lots of debug output. */
#define AP_MSG_LVL_VERBOSE		50

/** Selected level: */
#ifdef DEBUG
 #define AP_MSG_LVL AP_MSG_LVL_VERBOSE
#else
 #define AP_MSG_LVL AP_MSG_LVL_ERROR
#endif

/* Debug contexts*/
/** System related messages (default) */
#define AP_MSG_CTX_SYS	101

/** Handshake related messages */
#define AP_MSG_CTX_HS	102

/** State machine related messages */
#define AP_MSG_CTX_ST	103

/** Print all messages */
#define AP_MSG_CTX_ALL	100

/** Selected context: */
#define AP_MSG_CTX AP_MSG_CTX_ALL

// Number of bytes for the system command buffer.. 512 bytes should be more than enough
#define SYS_BUFSIZE 512

#define MANGLE_ICMP

/*********************** Definitions only relevant for Linux systems ************************/

#ifndef MACOSX

// If using linux, defining NOCHECKSUMS tells the tun/tap device not to
// use checksumming
//#define NOCHECKSUMS

// the name of the tun/tap device in /dev
#define TUNTAPDEVICE "/dev/net/tun"

// the network interfaces name (wildcard %d possible)
#define IFACENAME "alpha%d"

// Some non-default routing table number we want to use with iproute2
#define SYS_ROUTINGTABLE 23

// Some (hopefully) unique magic number for the iptables mangle table number we want to use
#define SYS_MARKTABLE 2342

// Absolute path of some important binaries
#define SYS_PREFIX "" // "sudo"
#define SYS_IPTABLES "/sbin/iptables"
#define SYS_IPROUTE "/sbin/ip"
#define SYS_IFCONFIG "/sbin/ifconfig"

#else

/****************** Definitions only relevant for Mac OS X / BSD systems ************************/

// Absolute path of some important binaries
#define SYS_PREFIX ""
#define SYS_IPFW "/sbin/ipfw"

// The port number for the divert port to use
#define SYS_DIVERT_PORT 12342

// The ipfw rule number
#define SYS_IPFW_RULE 2342

#endif

/****************** Definitions only r 11857 total
 * elevant for ALPHA MODES ************************/

//! Number of elements which can be stored (should be more than a association can recieve)
#define ALPHA_C_RING_BUFSIZE (MAX_PRESIG_COUNT * 2)
//! Number of elements a Alpha N association stores when a new S1 arrives before a S2 has come in (a value of less than 10 is appropiate)
#define ALPHA_N_RING_BUFSIZE 5

//! Minimum number of anchors before new associations are bootstrapped
#define MIN_ANCHORS 10

//! Must be at least 2! the number of sign elements which can be stored per association
#define ASS_SIGN_ANCHORS_BUFSIZE 10
//! the number of ack elements which can be stored per association
#define ASS_ACK_ANCHORS_BUFSIZE 10

#endif /* __DEFINES_H_ */
