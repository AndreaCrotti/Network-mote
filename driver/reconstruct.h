#ifndef _RECONSTRUCT_H
#define _RECONSTRUCT_H

// max number of packets kept in the temporary structure for reconstruction
#define MAX_RECONSTRUCTABLE 10

// this could be computed somehow from the MAX_ETHERNET and the size carried
#define MAX_CHUNKS 100

#include "util.h"

typedef struct {
    int seq_no;
    // what type here? more correct to allocate at run time once we know
    // of how many parts is the packet composed
    stream_t chunks[MAX_CHUNKS];
} packet_t;


void add_chunk(void* data);


#endif
