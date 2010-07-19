#include <stdio.h>
#include <stdlib.h>
#include "reconstruct.h"
#include "chunker.h"

// TODO: check that all the types are actually correct

// a simple function is not enough, we need an "object" which keeps the state
// of all the temporary packets and take from the outside the new packets we want to add.
// When one packet is ready it should go in another structure and maybe we can use a callback
// function to send it away automatically

// Maybe we could use this http://www.cl.cam.ac.uk/~cwc22/hashtable/ as data structure

// FIXME: now when the index changes we don't find it anymore
#define NEXT_POS(x) ((x + 1) % MAX_RECONSTRUCTABLE)

#ifndef DEBUG
#define DEBUG 0
#endif

myPacketHeader *recast(stream_t *data);
void move_forward(int seq_no);
void resetPacket(packet_t *actual, myPacketHeader *original);

// just using a send function would be fine
static void (*globalcallback)(myPacketHeader *completed);
static packet_t temp_packets[MAX_RECONSTRUCTABLE];
static packet_t completed_packets[MAX_RECONSTRUCTABLE];

static int index = 0;

// pass a callback function to send somewhere else the messages when they're over
void initReconstruction(void (*callback)(myPacketHeader *completed)) {
    int i, j;
    if (DEBUG)
        printf("initializing the reconstruction\n");

    globalcallback = callback;
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
    myPacketHeader *original = recast(data);
    int seq_no = original->seq_no;
    int ord_no = original->ord_no;
    
    // just for readability
    packet_t *actual = &temp_packets[index];

    // we have to overwrite everything in case we're overwriting OR
    // is the first chunk with that seq_no that we receive
    if (actual->seq_no != seq_no) {
        if (DEBUG)
            printf("overwriting or creating new packet at position %d\n", index);

        resetPacket(actual, original);
    }
    if (DEBUG) 
        printf("adding chunk ord_no = %d to seq_no = %d\n", ord_no, seq_no);

    actual->seq_no = seq_no;
    // check not receiving same data twice
    assert(actual->chunks[ord_no] == 0);
    // now add the chunk (using the payload
    /* actual.chunks[ord_no] = (stream_t *); */
    actual->missing_chunks--;

    index = NEXT_POS(index);
}

// reset all the chunks at that sequential number
void resetPacket(packet_t *actual, myPacketHeader *original) {
    int i;
    actual->seq_no = original->seq_no;
    actual->missing_chunks = original->parts;
        
    for (i = 0; i < MAX_CHUNKS; i++) {
        actual->chunks[i] = 0;
    }
}

// TODO: change this from myPacketHeader to the real structure we're getting
myPacketHeader *recast(stream_t *data) {
    return (myPacketHeader *) data;
}

#ifdef STANDALONE
int num_packets = 10;
void testAddressing();
void testRecast(myPacketHeader *p);

// doing some simple testing
int main(int argc, char *argv[]) {
    // give it a real function
    int i;
    initReconstruction(NULL);
    myPacketHeader *pkt = malloc(sizeof(myPacketHeader) * num_packets);
    
    for (i = 0; i < num_packets; i++) {
        testRecast(&pkt[i]);
        pkt[i].seq_no = i;
        addChunk(&pkt[i]);
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
    int i, seq;
    for (i = 0; i < num_packets; i++) {
        if (DEBUG)
            printf("checking packet %d where seq=%d max=%d\n", i, temp_packets[i].seq_no, MAX_RECONSTRUCTABLE);

        seq = temp_packets[i].seq_no;
        assert((seq % MAX_RECONSTRUCTABLE) == (i % MAX_RECONSTRUCTABLE));
    }
}

void testRecast(myPacketHeader *p) {
    myPacketHeader *other = malloc(sizeof(myPacketHeader));
    other = recast((void *) p);
    assert(other->seq_no == p->seq_no);
    assert(other->ord_no == p->ord_no);
}

#endif
