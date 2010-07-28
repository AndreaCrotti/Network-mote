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
int payloadEquals(payload_t x, payload_t y) {
    if (x.len != y.len) {
        printf("%d != %d\n", x.len, y.len);
        return 0;
    }
       
    for (unsigned i = 0; i < x.len; i++) {
        if (x.stream[i] != y.stream[i]) {
            printf("%d != %d\n", x.stream[i], y.stream[i]);
            return 0;
        }
    }
    return 1;
}

void copyPayload(payload_t *src, payload_t *dst) {
    // total dimension of the pointer and len are not the same thing actually
    //assert(src->len <= dst->len);
    dst->len = src->len;
    memcpy((void *) dst->stream, src->stream, dst->len);
}

myPacketHeader *getHeader(ipv6Packet *packet) {
    return &(packet->header.packetHeader);
}

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

// not really needed now since we already get the right size
int getSize(ipv6Packet *packet, int size) {
    // TODO check size
    (void)size;
    int computed_size;
    if (isLast(packet)) {
        // we need to invert from htons!!
#if !NO_IPV6 // This means IPv6 IS ENABLED!!!
        computed_size = ntohs(getPlen(packet)) - sizeof(myPacketHeader);
#else
        computed_size = size - sizeof(struct ipv6PacketHeader);
#endif
    } else {
        computed_size = MAX_CARRIED;
    }

    printf("computed_size is %d\n", computed_size);
    printf("MAX_CARRIED is %d\n", MAX_CARRIED);
    printf("size is %d\n", size);

    // TODO: Currently commmented out since mote transmit the maximum all the time
    assert((computed_size + sizeof(struct ipv6PacketHeader)) == (unsigned) size);
    return computed_size;
}
