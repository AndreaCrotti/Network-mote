/**
 * Some function utilities to print and manipulate our structures
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "util.h"

#include "structs.h"

void print_payload(payload_t t) {
    LOG_DEBUG("payload with length = %d", t.len);
}

void print_packet_header(myPacketHeader *pkt) {
    LOG_DEBUG("(seq = %d, ord = %d, parts = %d)", pkt->seq_no, pkt->ord_no, pkt->parts);
}

void make_myPacket(myPacket *packet, int seq_no, int ord_no, int parts, stream_t *payload, int len) {
    packet->packetHeader.seq_no = seq_no;
    packet->packetHeader.ord_no = ord_no;
    packet->packetHeader.parts = parts;
    // now also set the payload and it's length
    memcpy(payload, packet->payload, len);
}

/****************************************/
/* Functions to access to the structure */
/****************************************/
bool payload_equals(payload_t x, payload_t y) {
    if (x.len != y.len) {
        LOG_DEBUG("%d != %d", x.len, y.len);
        return 0;
    }
       
    for (unsigned i = 0; i < x.len; i++) {
        if (x.stream[i] != y.stream[i]) {
            LOG_DEBUG("%d != %d", x.stream[i], y.stream[i]);
            return false;
        }
    }
    // they are correct now
    return true;
}

void copy_payload(payload_t *src, payload_t *dst) {
    // total dimension of the pointer and len are not the same thing actually
    dst->len = src->len;
    memcpy((void *) dst->stream, src->stream, dst->len);
}

myPacketHeader *get_header(myPacket *packet) {
    return &(packet->packetHeader);
}

bool is_compressed(myPacket *packet) {
    return get_header(packet)->is_compressed;
}

bool is_last(myPacket *packet) {
    return (get_ord_no(packet) == (get_parts(packet) - 1));
}

int get_seq_no(myPacket *packet) {
    return get_header(packet)->seq_no;
}

int get_ord_no(myPacket *packet) {
    return get_header(packet)->ord_no;
}

int get_parts(myPacket *packet) {
   return get_header(packet)->parts;
}

int get_size(myPacket *packet, int size) {
    int computed_size;
    if (is_last(packet)) {
        computed_size = size - sizeof(myPacketHeader);
    } else {
        computed_size = MAX_CARRIED;
    }

    assert((computed_size + sizeof(myPacketHeader)) == (unsigned) size);
    return computed_size;
}
