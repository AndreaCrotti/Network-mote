#ifndef STRUCTS_H
#define STRUCTS_H

// measures in bytes
#define SIZE_IPV6_HEADER 40
// For the usage of additional headers MAX_CARRIED has to be smaller
#define MAX_CARRIED (TOSH_DATA_LENGTH - sizeof(struct ipv6PacketHeader))
#define TOT_PACKET_SIZE(payload_len) (sizeof(struct ipv6PacketHeader) + payload_len)
#define PAYLOAD_LEN (MAX_CARRIED - sizeof(ipv6Packet))

#define TUNTAP_INTERFACE IFF_TUN

#if TUNTAP_INTERFACE == IFF_TAP
#define MAX_FRAME_SIZE 2048
#elif TUNTAP_INTERFACE == IFF_TUN
#define MAX_FRAME_SIZE 65536
#else
#error "Unsupported tun/tap interface."
#endif

/// how many clients can the gateway manage
#define MAX_CLIENTS 10

#define COPY_STRUCT (DESTP, SOURCEP, TYPE) *(TYPE*)(DESTP) = *(TYPE*)(SOURCEP)

#include <stdbool.h>
#include <ip.h>

typedef unsigned char stream_t;
typedef unsigned int streamlen_t;

typedef struct {
    stream_t const* stream;
    streamlen_t len;
    bool is_compressed;
} payload_t;

typedef struct in6_addr in6_src;
typedef struct in6_addr in6_dst;

// just for more ease of writing
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct ip6_hdr ip6_hdr;

typedef uint8_t seq_no_t;
typedef uint16_t am_addr_t;

// also the internal struct should be packed
typedef struct myPacketHeader {
    seq_no_t seq_no;
    uint8_t ord_no;
    // tells if the payload is compressed or not
    bool is_compressed;
    // how many chunks in total
    uint8_t parts;
} __attribute__((__packed__)) myPacketHeader;

// only the final ipv6 packet must be "__packed__".
typedef struct ipv6Packet {
    // sent data ...
    struct ipv6PacketHeader{
#if !NO_IPV6 
        struct ip6_hdr ip6_hdr;
#endif
        myPacketHeader packetHeader;
    } __attribute__((__packed__)) header;

    stream_t payload[MAX_CARRIED];
} __attribute__((__packed__)) ipv6Packet;

// XXX XXX XXX HACK
struct dummy {
  char bla[102];
};

/*********************************************/
/* some useful functions to print structures */
/*********************************************/

/** 
 * Check if the two payloads are equal, of course the length must be correct
 * 
 */
void print_payload(payload_t t);
void print_packet_header(myPacketHeader *pkt);
void print_ipv6Header(ip6_hdr header);
void print_ipv6Packet(ipv6Packet *pkt);

/** 
 * Create a ipv6packet with those values, mainly for testing and debugging
 * 
 */
void make_ipv6Packet(ipv6Packet *ip6_pkt, int seq_no, int ord_no, int parts, stream_t *payload, int len);

/****************************************************************/
/* accessing to the internal data of the structures more easily */
/****************************************************************/

/**
 * Copy a payload structure, size of destiantion must be greater or equal
 */
void copy_payload(payload_t *src, payload_t *dst);

/** 
 * Check if the two payloads are equal, of course the length must be correct
 * 
 */
int payload_equals(payload_t x, payload_t y);

/** 
 * Returns a pointer to our own header
 */
myPacketHeader *getHeader(ipv6Packet *packet);

/*************************************************/
/* Accessing to internal fields of the structure */
/*************************************************/
int get_size(ipv6Packet *packet, int size);
int get_parts(ipv6Packet *packet);
int get_ord_no(ipv6Packet *packet);
int get_seq_no(ipv6Packet *packet);
bool is_compressed(ipv6Packet *packet);

#endif

