#ifndef __CHUNKER_H
#define __CHUNKER_H
#include <stdint.h>
#include "util.h"
/* #include <netinet/ip6.h> */
#include <ip.h>

// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED TOSH_DATA_LENGTH
#define TOT_PACKET_SIZE(payload_len) (sizeof(ipv6PacketHeader) + payload_len)
#define PAYLOAD_LEN (MAX_CARRIED - sizeof(ipv6Packet))

typedef struct in6_addr in6_src;
typedef struct in6_addr in6_dst;

// TODO: add another field to keep the total number of parts needed
// also the internal struct should be packed
typedef struct myPacketHeader {
    uint8_t seq_no;
    uint8_t ord_no;
    uint8_t parts;
    unsigned short checksum;
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
    // END OF sent data.
    // extra data ...
    unsigned sendsize;
    unsigned plsize;
} __attribute__((__packed__)) ipv6Packet;

// just for more ease of writing
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct ip6_hdr ip6_hdr;

/** 
 * Generates an array of ipv6Packet
 * 
 * @param void * pointer to the data
 * @param int number of chunks (must be computed externally)
 * @param int sequential number of this packet
 * 
 * @return 
 */
int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, int const seq_no);

/** 
 * Reconstruct the stream of data given the ipv6 packets
 * 
 * 
 * @return 
 */
void *reconstruct(ipv6Packet *data, int len);

#endif
