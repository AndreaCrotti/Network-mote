#ifndef __STRUCTS_H
#define __STRUCTS_H

// measures in bytes
#define SIZE_IPV6_HEADER 40
// For the usage of additional headers MAX_CARRIED has to be smaller
#define MAX_CARRIED (TOSH_DATA_LENGTH - sizeof(struct ipv6PacketHeader))
//#define MAX_CARRIED (TOSH_DATA_LENGTH - sizeof(ipv6PacketHeader) - MCP_HEADER_BYTES - 5)
#define TOT_PACKET_SIZE(payload_len) (sizeof(struct ipv6PacketHeader) + payload_len)
#define PAYLOAD_LEN (MAX_CARRIED - sizeof(ipv6Packet))
// TODO: change it to max ipv4 packet length
#define MAX_ETHERNET_FRAME_SIZE 2048
// see total length for ipv4
/* Total Length  */
/* This 16-bit field defines the entire datagram size, including header and data, in bytes. The minimum-length datagram is 20 bytes (20-byte header + 0 bytes data) and the maximum is 65,535 — the maximum value of a 16-bit word. The minimum size datagram that any host is required to be able to handle is 576 bytes, but most modern hosts handle much larger packets. Sometimes subnetworks impose further restrictions on the size, in which case datagrams must be fragmented. Fragmentation is handled in either the host or packet switch in IPv4 (see Fragmentation and reassembly). */

// max number of clients we support
#define MAX_CLIENTS 10

#include <ip.h>

typedef unsigned char stream_t;
typedef unsigned int streamlen_t;

typedef struct {
  stream_t const* stream;
  streamlen_t len;
} payload_t;

typedef struct in6_addr in6_src;
typedef struct in6_addr in6_dst;

// just for more ease of writing
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct ip6_hdr ip6_hdr;

// TODO: add another field to keep the total number of parts needed
// also the internal struct should be packed
typedef struct myPacketHeader {
    uint8_t seq_no;
    uint8_t ord_no;
    uint8_t parts;
} __attribute__((__packed__)) myPacketHeader;

// FIXME: ipv6 header is not needed inside here anymore
// only the final ipv6 packet must be "__packed__".
typedef struct ipv6Packet {
    // sent data ...
    struct ipv6PacketHeader{ 
        struct ip6_hdr ip6_hdr;
        myPacketHeader packetHeader;
    } __attribute__((__packed__)) header;

    stream_t payload[MAX_CARRIED];
} __attribute__((__packed__)) ipv6Packet;

#endif

