#include "reconstruct.h"

// a simple function is not enough, we need an "object" which keeps the state
// of all the temporary packets and take from the outside the new packets we want to add.
// When one packet is ready it should go in another structure and maybe we can use a callback
// function to send it away automatically

// Maybe we could use this http://www.cl.cam.ac.uk/~cwc22/hashtable/ as data structure


void add_chunk(void *data) {
    static packet_t temp_packets[MAX_RECONSTRUCTABLE];
}
