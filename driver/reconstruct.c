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

void recast(ipv6Packet *, stream_t *data);
void reset_packet(packet_t *actual, ipv6Packet *original);
int get_ord_no(ipv6Packet *packet);
int get_seq_no(ipv6Packet *packet);
int get_parts(ipv6Packet *packet);
int is_last(ipv6Packet *packet);
int get_plen(ipv6Packet *packet);
packet_t *get_packet(int seq_no);
stream_t *reconstruct(stream_t *result, int seq_no);

myPacketHeader *get_header(ipv6Packet *packet);

// just using a send function would be fine
static void (*send_back)(ipv6Packet *completed);
static packet_t temp_packets[MAX_RECONSTRUCTABLE];

// pass a callback function to send somewhere else the messages when they're over
void initReconstruction(void (*callback)(ipv6Packet *completed)) {
    int i, j;
    if (DEBUG)
        printf("initializing the reconstruction\n");

    send_back = callback;
    for (i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        // is it always a new packet_t right?
        packet_t t;
        t.seq_no = -1;
        // set to 0 all the chunks
        for (j = 0; j < MAX_CHUNKS; j++) {
            t.chunks[j] = 0;
        }
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
    // doing a memcpy instead
    memcpy(original, data, sizeof(ipv6Packet));
    int seq_no = get_seq_no(original);
    int ord_no = get_ord_no(original);
    
    // just for readability
    packet_t *actual = &temp_packets[POS(seq_no)];

    // we have to overwrite everything in case we're overwriting OR
    // is the first chunk with that seq_no that we receive
    if (actual->seq_no != seq_no) {
        if (DEBUG)
            printf("overwriting or creating new packet at position %d\n", POS(seq_no));

        reset_packet(actual, original);
    }
    if (DEBUG) 
        printf("adding chunk seq_no = %d\n", seq_no);

    actual->seq_no = seq_no;

    // this is to make sure that we don't decrement missing_chunks even when not adding
    if (actual->chunks[ord_no] == 0) {
        // fetch the real data of the payload, check if it's the last one
        int size;

        if (is_last(original)) {
            size = get_plen(original) - sizeof(original->header);
        } else {
            size = MAX_CARRIED - sizeof(myPacketHeader);
        }
        memcpy(&(actual->chunks), &(original->payload), size);
        actual->missing_chunks--;
    } else {
        printf("we got the same chunk twice!!!\n");
    }

    // now we check if everything if the packet is completed and sends it back
    if (actual->missing_chunks == 0) {
        send_back(original);
    }
    free(original);
}

// reconstruct the completed packet, what we get from here should be
// a perfectly valid Ethernet frame
stream_t *reconstruct(stream_t *result, int seq_no) {
    int i;
    // what do we have to do?? Add them together or what a memcpy maybe?
    for (i = 0; i < MAX_RECONSTRUCTABLE; i++) {
    }
    return result;
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

// reset all the chunks at that sequential number
void reset_packet(packet_t *actual, ipv6Packet *original) {
    int i;
    actual->seq_no = get_seq_no(original);
    actual->missing_chunks = get_parts(original);
        
    for (i = 0; i < MAX_CHUNKS; i++) {
        actual->chunks[i] = 0;
    }
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
    if (get_ord_no(packet) == get_parts(packet)) {
        return 1;
    } else {
        return 0;
    }
}

int get_seq_no(ipv6Packet *packet) {
    return get_header(packet)->seq_no;
}

int get_ord_no(ipv6Packet *packet) {
    return get_header(packet)->ord_no;
}

int get_plen(ipv6Packet *packet) {
    return packet->header.ip6_hdr.plen;
}

int get_parts(ipv6Packet *packet) {
   return get_header(packet)->parts;
}

void make_ipv6_packet(ipv6Packet *packet, int seq_no, int ord_no) {
    packet->header.packetHeader.seq_no = seq_no;
    packet->header.packetHeader.ord_no = ord_no;
}

#ifdef STANDALONE
int num_packets = 10;
void testAddressing();
void testRecast(ipv6Packet *p);

// doing some simple testing
int main(int argc, char *argv[]) {
    // give it a real function
    int i;
    initReconstruction(NULL);
    ipv6Packet *pkt = calloc(num_packets, sizeof(ipv6Packet));
    
    for (i = 0; i < num_packets; i++) {
        make_ipv6_packet(&(pkt[i]), i, 0);
        addChunk((void *) &pkt[i]);
    }

    testAddressing();

    // assertions to check we really have those values there
    // check 
    free(pkt);
    return 0;
}

// at every position there should be something such that
// (POS % seq_no) == 0
void testAddressing() {
    int i;
    for (i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        packet_t *actual = &(temp_packets[i]);
        assert((actual->seq_no % MAX_RECONSTRUCTABLE) == i);
   }
}

#endif
