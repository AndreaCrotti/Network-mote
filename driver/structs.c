/**
 * Some function utilities to print and manipulate our structures
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "util.h"

#include "structs.h"

void print_payload(payload_t t) {
    LOG_DEBUG("payload with length = %d", t.len);
}

void print_packet_header(myPacketHeader *pkt) {
    LOG_DEBUG("(seq = %d, ord = %d, parts = %d)", pkt->seq_no, pkt->ord_no, pkt->parts);
}

void print_ipv6Header(ip6_hdr header) {
    LOG_DEBUG("len = %d, src...", header.plen);
}

void print_ipv6Packet(ipv6Packet *pkt) {
    print_packet_header(&((pkt->header).packetHeader));
#if !NO_IPV6
    print_ipv6Header((pkt->header).ip6_hdr);
#endif
}

// only used for testing out something
void make_ipv6Packet(ipv6Packet *packet, int seq_no, int ord_no, int parts, stream_t *payload, int len) {
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
int payload_equals(payload_t x, payload_t y) {
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

void copy_payload(payload_t *src, payload_t *dst) {
    // total dimension of the pointer and len are not the same thing actually
    dst->len = src->len;
    memcpy((void *) dst->stream, src->stream, dst->len);
}

myPacketHeader *get_header(ipv6Packet *packet) {
    return &(packet->header.packetHeader);
}

bool is_compressed(ipv6Packet *packet) {
    return get_header(packet)->is_compressed;
}

bool is_last(ipv6Packet *packet) {
    return (get_ord_no(packet) == (get_parts(packet) - 1));
}

int get_seq_no(ipv6Packet *packet) {
    return get_header(packet)->seq_no;
}

int get_ord_no(ipv6Packet *packet) {
    return get_header(packet)->ord_no;
}

int get_plen(ipv6Packet *packet) {
    int plen;
    (void)packet;
#if !NO_IPV6
    plen = packet->header.ip6_hdr.plen;
// FIXME: this dirty thing is just to always return something, clean it
#else
    plen = 0;
    assert(0);
#endif
    return plen;
}

int get_parts(ipv6Packet *packet) {
   return get_header(packet)->parts;
}

// not really needed now since we already get the right size
int get_size(ipv6Packet *packet, int size) {
    // TODO check size
    int computed_size;
    if (is_last(packet)) {
        // we need to invert from htons!!
#if !NO_IPV6
        computed_size = ntohs(get_plen(packet)) - sizeof(myPacketHeader);
#else
        computed_size = size - sizeof(struct ipv6PacketHeader);
#endif
    } else {
        computed_size = MAX_CARRIED;
    }

    assert((computed_size + sizeof(struct ipv6PacketHeader)) == (unsigned) size);
    return computed_size;
}
