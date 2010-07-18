#include <stdio.h>

#include "reconstruct.h"
#include "chunker.h"

// a simple function is not enough, we need an "object" which keeps the state
// of all the temporary packets and take from the outside the new packets we want to add.
// When one packet is ready it should go in another structure and maybe we can use a callback
// function to send it away automatically

// Maybe we could use this http://www.cl.cam.ac.uk/~cwc22/hashtable/ as data structure

#define MAX_SEQ (index + MAX_RECONSTRUCTABLE)
#define POS(x) (x - index)

myPacketHeader *recast(stream_t *data);
void move_forward(int seq);

static void (*globalcallback)(myPacketHeader *completed);
static packet_t temp_packets[MAX_RECONSTRUCTABLE];
static packet_t completed_packets[MAX_RECONSTRUCTABLE];

static int index = 0;

// pass a callback function to send somewhere else the messages when they're over
void initReconstruction(void (*callback)(myPacketHeader *completed)) {
    int i, j;
    globalcallback = callback;
    for (i = 0; i < MAX_RECONSTRUCTABLE; i++) {
        // is it always a new packet_t right?
        packet_t t;
        t.seq_no = -1;
        // set to 0 all the chunks
        for (i = 0; j < MAX_CHUNKS; j++) {
            t.chunks[j] = 0;
        }
        temp_packets[i] = t;
    }
}

void addChunk(void *data) {
    myPacketHeader *p = recast(data);
    int seq_no = p->seq_no;
    int ord_no = p->ord_no;
    // does nothing if we're in the limit
    move_forward(seq_no);
    
    // just for readability
    packet_t *actual = &temp_packets[POS(seq_no)];
    // check if first time we receive a chunk of this packet
    if (actual->seq_no >= 0) {
        actual->missing_chunks = p->parts;
        actual->seq_no = seq_no;
    }
    
    // not receiving same data twice
    assert(actual->chunks[ord_no] == 0);
    // now add the chunk (using the payload
    /* actual.chunks[ord_no] = (stream_t *); */
    actual->missing_chunks--;
}

void move_forward(int seq) {
    int offset = (seq - MAX_SEQ);
    if (offset > 0) {
        printf("we'll overwrite everything below %d\n", offset);
        index += offset;
    }
}

// TODO: change this from myPacketHeader to the real structure we're getting
myPacketHeader *recast(stream_t *data) {
    return (myPacketHeader *) data;
}

#ifdef STANDALONE

int main(int argc, char *argv[]) {
    
}

#endif
