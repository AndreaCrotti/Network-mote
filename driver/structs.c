/**
 * Some function utilities to debug structs
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../shared/structs.h"

void printPayload(payload_t t) {
    printf("payload with length = %d\n", t.len);
}

void printPacketHeader(myPacketHeader *pkt) {
    printf("(seq = %d, ord = %d, parts = %d)\n", pkt->seq_no, pkt->ord_no, pkt->parts);
}

void printIpv6Header(ip6_hdr header) {
    printf("len = %d, src...\n", header.plen);
}

void printIpv6Packet(ipv6Packet *pkt) {
    printPacketHeader(&((pkt->header).packetHeader));
#if !NO_IPV6
    printIpv6Header((pkt->header).ip6_hdr);
#endif
}

// only used for testing out something
void makeIpv6Packet(ipv6Packet *packet, int seq_no, int ord_no, int parts, stream_t *payload, int len) {
    packet->header.packetHeader.seq_no = seq_no;
    packet->header.packetHeader.ord_no = ord_no;
    packet->header.packetHeader.parts = parts;
    // now also set the payload and it's length
#if !NO_IPV6
    packet->header.ip6_hdr.plen = len + sizeof(packet->header);
#endif
    memcpy(payload, packet->payload, len);
}


/****************************************/
/* Functions to access to the structure */
/****************************************/

myPacketHeader *getHeader(ipv6Packet *packet) {
    return &(packet->header.packetHeader);
}

/** 
 * Check if this is the last chunk 
 * 
 * @param packet 
 * 
 * @return 1 if it's last, 0 otherwise
 */
int isLast(ipv6Packet *packet) {
    return (getOrdNo(packet) == (getParts(packet) - 1));
}

int getSeqNo(ipv6Packet *packet) {
    return getHeader(packet)->seq_no;
}

int getOrdNo(ipv6Packet *packet) {
    return getHeader(packet)->ord_no;
}

int getPlen(ipv6Packet *packet) {
    int plen;
#if !NO_IPV6
    plen = packet->header.ip6_hdr.plen;
#else
    plen = 0;
    (void)packet;
    assert(0);
#endif
    return plen;
}

int getParts(ipv6Packet *packet) {
   return getHeader(packet)->parts;
}

int getSize(ipv6Packet *packet, int size) {
    // TODO check size
    (void)size;
    int computed_size;
    if (isLast(packet)) {
        // we need to invert from htons!!
#if !NO_IPV6
        computed_size = ntohs(getPlen(packet)) - sizeof(myPacketHeader);
#else
        computed_size = size - sizeof(struct ipv6PacketHeader);
#endif
    } else {
        computed_size = MAX_CARRIED;
    }
    // TODO: Currently commmented out since mote transmit the maximum all the time
    assert((computed_size + sizeof(struct ipv6PacketHeader)) == (unsigned) size);
    return computed_size;
}
