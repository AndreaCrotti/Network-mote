#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reconstruct.h"
#include "chunker.h"

// TODO: check that all the types are actually correct
// TODO: (if there is time) add multiclient support (reading the source and create temp structure for both)
// TODO: check with valgrind

// a simple function is not enough, we need an "object" which keeps the state
// of all the temporary packets and take from the outside the new packets we want to add.
// When one packet is ready it should go in another structure and maybe we can use a callback
// function to send it away automatically

// Maybe we could use this http://www.cl.cam.ac.uk/~cwc22/hashtable/ as data structure

// FIXME: now when the index changes we don't find it anymore
#define POS(x) (x % MAX_RECONSTRUCTABLE)

#ifndef DEBUG
#define DEBUG 0
#endif

void reset_packet(packet_t *actual);
int get_ord_no(ipv6Packet *packet);
int get_seq_no(ipv6Packet *packet);
int get_parts(ipv6Packet *packet);
int is_last(ipv6Packet *packet);
int get_plen(ipv6Packet *packet);
int is_completed(packet_t *pkt);
void make_ipv6_packet(ipv6Packet *packet, int seq_no, int ord_no, int parts, stream_t *payload, int len);
packet_t *get_packet(int seq_no);

myPacketHeader *get_header(ipv6Packet *packet);

// just using a send function would be fine
static void (*send_back)(ipv6Packet *completed);
static packet_t temp_packets[MAX_RECONSTRUCTABLE];

// pass a callback function to send somewhere else the messages when they're over
void initReconstruction(void (*callback)(ipv6Packet *completed)) {
    if (DEBUG)
        printf("initializing the reconstruction\n");

    send_back = callback;
    for (int i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        // is it always a new packet_t right?
        packet_t t;
        t.seq_no = -1;
        temp_packets[i] = t;
    }
}

/** 
 * Add a new chunk to the list of temp
 * 
 * @param data 
 */
void addChunk(void *data) {
    ipv6Packet *original = malloc(sizeof(ipv6Packet));
    memcpy(original, data, sizeof(ipv6Packet));
    int seq_no = get_seq_no(original);
    int ord_no = get_ord_no(original);
    
    // just for readability
    packet_t *actual = &temp_packets[POS(seq_no)];

    // we have to overwrite everything in case we're overwriting OR
    // is the first chunk with that seq_no that we receive
    // TODO: use get_packet instead to check this condition
    if (actual->seq_no != seq_no) {
        if (DEBUG)
            printf("overwriting or creating new packet at position %d\n", POS(seq_no));

        
        actual->completed_bitmask = (1 << get_parts(original)) - 1;
        actual->tot_size = 0;
        actual->seq_no = seq_no;
    }

    // this is to make sure that we don't decrement missing_chunks even when not adding
    // fetch the real data of the payload, check if it's the last one
    int size;

    if (is_last(original)) {
        printf("in last chunk\n");
        size = get_plen(original) - sizeof(original->header);
    } else {
        size = MAX_CARRIED;
    }

    // we can always do this since only the last one is not fullsize
    memcpy(actual->chunks + (MAX_CARRIED * ord_no), original->payload, size);
    
    (actual->completed_bitmask) &= ~(1 << ord_no);

    // now we check if everything if the packet is completed and sends it back

    if (is_completed(actual)) {
        if (DEBUG)
            printf("packet with seq_no %d completed\n", seq_no);
    }

    free(original);
}

int is_completed(packet_t *pkt) {
    return (pkt->completed_bitmask == 0);
}

/** 
 * @param seq_no sequential number to look for
 * 
 * @return NULL if not found, the pointer if found
 *         It can only returns null if that seq_no has been already overwritten
 */
packet_t *get_packet(int seq_no) {
    packet_t *found = &temp_packets[POS(seq_no)];
    if (found->seq_no == seq_no) {
        return found;
    }
    return NULL;
}

/****************************************/
/* Functions to access to the structure */
/****************************************/

myPacketHeader *get_header(ipv6Packet *packet) {
    return &(packet->header.packetHeader);
}

/** 
 * Check if this is the last chunk 
 * 
 * @param packet 
 * 
 * @return 1 if it's last, 0 otherwise
 */
int is_last(ipv6Packet *packet) {
    return (get_ord_no(packet) == (get_parts(packet) - 1));
}

int get_seq_no(ipv6Packet *packet) {
    return get_header(packet)->seq_no;
}

int get_ord_no(ipv6Packet *packet) {
    return get_header(packet)->ord_no;
}

int get_plen(ipv6Packet *packet) {
    int plen = packet->header.ip6_hdr.plen;
    if (DEBUG)
        printf("got length = %d\n", plen);
    return plen;
}

int get_parts(ipv6Packet *packet) {
   return get_header(packet)->parts;
}

// only used for testing out something
void make_ipv6_packet(ipv6Packet *packet, int seq_no, int ord_no, int parts, stream_t *payload, int len) {
    packet->header.packetHeader.seq_no = seq_no;
    packet->header.packetHeader.ord_no = ord_no;
    packet->header.packetHeader.parts = parts;
    // now also set the payload and it's length
    packet->header.ip6_hdr.plen = len + sizeof(packet->header);
    memcpy(payload, packet->payload, len);
}

#ifdef STANDALONE
int num_packets = 10;
void test_addressing();
void test_last();

// doing some simple testing
int main(int argc, char *argv[]) {
    // give it a real function
    initReconstruction(NULL);
    ipv6Packet *pkt = calloc(num_packets, sizeof(ipv6Packet));
    
    // create some fake payload to create correctly the payload
    
    test_last();
    stream_t x[2] = {0, 1};

    for (int i = 0; i < num_packets; i++) {
        make_ipv6_packet(&(pkt[i]), 0, i, num_packets, x, 2);
        addChunk((void *) &pkt[i]);
    }

    // 0 for example can't be found
    assert(get_packet(0) == NULL);

    test_addressing();

    // assertions to check we really have those values there
    // check 
    free(pkt);
    return 0;
}

// at every position there should be something such that
// ( POS % seq_no) == 0
void test_addressing() {
    for (int i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        packet_t *actual = &(temp_packets[i]);
        assert((actual->seq_no % MAX_RECONSTRUCTABLE) == i);
   }
}

void test_last() {
    ipv6Packet *pkt = malloc(sizeof(ipv6Packet));
    stream_t *payload = malloc(0);
    make_ipv6_packet(pkt, 0, 0, 1, payload, 0);
    assert(is_last(pkt));
    free(pkt);
}

#endif
