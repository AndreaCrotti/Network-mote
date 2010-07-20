#ifndef _RECONSTRUCT_H
#define _RECONSTRUCT_H

#include "structs.h"

// max number of packets kept in the temporary structure for reconstruction
#define MAX_RECONSTRUCTABLE 4

// this could be computed somehow from the MAX_ETHERNET and the size carried
#define MAX_CHUNKS 100

#include "util.h"

typedef struct {
    int seq_no;
    int missing_chunks; // when this goes to 0 we're done
    // MAX_CHUNKS should be instead the number of parts
    stream_t chunks[MAX_CHUNKS];
    // it normally is the max size of all the chunks + the size of the last one
    int tot_size;
} packet_t;


/** 
 * Initialize the reconstruction of packets
 * 
 * @param callback takes a function which will send back packets when they're completed
 */
void initReconstruction(void (*callback)(ipv6Packet *completed));

void addChunk(void *data);

#endif
