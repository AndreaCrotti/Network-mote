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

void print_packet_header(my_packet_header *pkt) {
    LOG_DEBUG("(seq = %d, ord = %d, parts = %d)", pkt->seq_no, pkt->ord_no, pkt->parts);
}

void make_my_packet(my_packet *packet, int seq_no, int ord_no, int parts, stream_t *payload, int len) {
    packet->packet_header.seq_no = seq_no;
    packet->packet_header.ord_no = ord_no;
    packet->packet_header.parts = parts;
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

my_packet_header *get_header(my_packet *packet) {
    return &(packet->packet_header);
}

bool is_compressed(my_packet *packet) {
    return get_header(packet)->is_compressed;
}

bool is_last(my_packet *packet) {
    return (get_ord_no(packet) == (get_parts(packet) - 1));
}

int get_seq_no(my_packet *packet) {
    return get_header(packet)->seq_no;
}

int get_ord_no(my_packet *packet) {
    return get_header(packet)->ord_no;
}

int get_parts(my_packet *packet) {
   return get_header(packet)->parts;
}

int get_size(my_packet *packet, int size) {
    int computed_size;
    if (is_last(packet)) {
        computed_size = size - sizeof(my_packet_header);
    } else {
        computed_size = MAX_CARRIED;
    }

    assert((computed_size + sizeof(my_packet_header)) == (unsigned) size);
    return computed_size;
}
