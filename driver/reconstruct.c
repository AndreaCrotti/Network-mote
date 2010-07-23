#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reconstruct.h"
#include "chunker.h"
#include "tunnel.h"

// TODO: check that all the types are actually correct
// TODO: check with valgrind

// a simple function is not enough, we need an "object" which keeps the state
// of all the temporary packets and take from the outside the new packets we want to add.
// When one packet is ready it should go in another structure and maybe we can use a callback
// function to send it away automatically

#define POS(x) (x % MAX_RECONSTRUCTABLE)

#ifndef DEBUG
#define DEBUG 0
#endif

void reset_packet(packet_t *pkt);
int get_ord_no(ipv6Packet *ip6_pkt);
int get_seq_no(ipv6Packet *ip6_pkt);
int get_parts(ipv6Packet *ip6_pkt);
int get_size(ipv6Packet *ip6_pkt);
int is_last(ipv6Packet *ip6_pkt);
int get_plen(ipv6Packet *ip6_pkt);
int is_completed(packet_t *pkt);
void send_if_completed(packet_t *pkt, int new_bm);
void make_ipv6_packet(ipv6Packet *ip6_pkt, int seq_no, int ord_no, int parts, stream_t *payload, int len);
packet_t *get_packet(int seq_no);

myPacketHeader *get_header(ipv6Packet *ip6_pkt);

// just using a send function would be fine
//static void (*send_back)(ipv6Packet *completed);
static packet_t temp_packets[MAX_RECONSTRUCTABLE];

/** 
 * Initializing the reconstruction module
 * 
 */
void initReconstruction(void) { // (*callback)(ipv6Packet *completed)) {
    if (DEBUG)
        printf("initializing the reconstruction\n");

    /* send_back = callback; */
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
    // casting the structure to the original type
    ipv6Packet *original = malloc(sizeof(ipv6Packet));
    memcpy(original, data, sizeof(ipv6Packet));
    int seq_no = get_seq_no(original);
    int ord_no = get_ord_no(original);
    
    // just for readability
    packet_t *pkt = &temp_packets[POS(seq_no)];
    
    if (DEBUG)
        printf("adding chunk (%d, %d, %d, %d)\n", seq_no, ord_no, get_parts(original), pkt->completed_bitmask);

    if (pkt->seq_no != seq_no) {
        if (DEBUG)
            printf("overwriting or creating new packet at position %d\n", POS(seq_no));
        
        // resetting to the initial configuration
        pkt->completed_bitmask = (1 << get_parts(original)) - 1;
        pkt->tot_size = 0;
        pkt->seq_no = seq_no;
    }

    // this is to make sure that we don't decrement missing_chunks even when not adding
    // fetch the real data of the payload, check if it's the last one
    int size = get_size(original);
    pkt->tot_size += size;

    if (DEBUG)
        printf("size = %d\n", size);

    // we can always do this since only the last one is not fullsize
    memcpy(pkt->chunks + (MAX_CARRIED * ord_no), original->payload, size);

    int new_bm = (pkt->completed_bitmask) & ~(1 << ord_no);
    send_if_completed(pkt, new_bm);

    free(original);
}

stream_t *getChunks(int seq_no) {
    packet_t *pkt = get_packet(seq_no);
    if (pkt)
        return pkt->chunks;

    return NULL;
}


int is_completed(packet_t *pkt) {
    return (pkt->completed_bitmask == 0);
}

// TODO: change name or change what is done inside here
void send_if_completed(packet_t *pkt, int new_bm) {
    if (new_bm == pkt->completed_bitmask)
        printf("adding twice the same chunk!!!!\n");
    else 
        pkt->completed_bitmask = new_bm;

    // now we check if everything if the packet is completed and sends it back

    if (is_completed(pkt)) {
        if (DEBUG)
            printf("packet completed\n");
        // TODO: implement the sending (writing on tap0 probably?)
        /* tun_write(getFd(), (char *) pkt->chunks, pkt->tot_size); */
        /* tun_write(getFd(), (char *) */
    }
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
    return plen;
}

int get_parts(ipv6Packet *packet) {
   return get_header(packet)->parts;
}

int get_size(ipv6Packet *packet) {
    if (is_last(packet)) {
        // we need to invert from htons!!
        return (ntohs(get_plen(packet)) - sizeof(myPacketHeader));
    } else {
        return MAX_CARRIED;
    }
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
    initReconstruction();
    ipv6Packet *pkt = calloc(num_packets, sizeof(ipv6Packet));
    
    // create some fake payload to create correctly the payload
    
    test_last();
    stream_t x[2] = {0, 1};

    for (int i = 0; i < num_packets; i++) {
        make_ipv6_packet(&(pkt[i]), 0, i, num_packets, x, 2);
        addChunk((void *) &pkt[i]);
    }

    get_packet(0);
    // 0 for example can't be found
    /* assert(get_packet(0) == NULL); */

    test_addressing();

    // TODO: add assertions for correct tot_size and more

    free(pkt);
    return 0;
}

// at every position there should be something such that
// ( POS % seq_no) == 0
void test_addressing() {
    for (int i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        packet_t *pkt = &(temp_packets[i]);
        assert((pkt->seq_no % MAX_RECONSTRUCTABLE) == i);
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
