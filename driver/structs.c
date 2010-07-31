/**
 * Some function utilities to print and manipulate our structures
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

#include "../shared/structs.h"

void printPayload(payload_t t) {
    (void)t;
    LOG_DEBUG("payload with length = %d", t.len);
}

void printPacketHeader(myPacketHeader *pkt) {
    (void)pkt;
    LOG_DEBUG("(seq = %d, ord = %d, parts = %d)", pkt->seq_no, pkt->ord_no, pkt->parts);
}

void printIpv6Header(ip6_hdr header) {
    (void)header;
    LOG_DEBUG("len = %d, src...", header.plen);
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
        LOG_DEBUG("%d != %d", x.len, y.len);
        return 0;
    }
       
    for (unsigned i = 0; i < x.len; i++) {
        if (x.stream[i] != y.stream[i]) {
            LOG_DEBUG("%d != %d", x.stream[i], y.stream[i]);
            return 0;
        }
    }
    // they are correct now
    return 1;
}

void copyPayload(payload_t *src, payload_t *dst) {
    // total dimension of the pointer and len are not the same thing actually
    dst->len = src->len;
    memcpy((void *) dst->stream, src->stream, dst->len);
}

myPacketHeader *getHeader(ipv6Packet *packet) {
    return &(packet->header.packetHeader);
}

bool is_compressed(ipv6Packet *packet) {
    return getHeader(packet)->is_compressed;
}

bool isLast(ipv6Packet *packet) {
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
// FIXME: this dirty thing is just to always return something, clean it
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

    // TODO: Currently commmented out since mote transmit the maximum all the time
    assert((computed_size + sizeof(struct ipv6PacketHeader)) == (unsigned) size);
    return computed_size;
}
