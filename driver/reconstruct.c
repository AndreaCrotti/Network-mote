#include "reconstruct.h"
#include "chunker.h"

// a simple function is not enough, we need an "object" which keeps the state
// of all the temporary packets and take from the outside the new packets we want to add.
// When one packet is ready it should go in another structure and maybe we can use a callback
// function to send it away automatically

// Maybe we could use this http://www.cl.cam.ac.uk/~cwc22/hashtable/ as data structure

myPacketHeader *recast(stream_t *data);

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

}

// TODO: change this from myPacketHeader to the real structure we're getting
myPacketHeader *recast(stream_t *data) {
    return (myPacketHeader *) data;
}

