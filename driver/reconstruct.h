#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#include "structs.h"

// max number of packets kept in the temporary structure for reconstruction
#define MAX_RECONSTRUCTABLE 100

// this could be computed somehow from the MAX_FRAME_SIZE and the size carried
#define MAX_CHUNKS 100

#include "util.h"

/** 
 * Initialize the reconstruction of packets
 * 
 * @param callback takes a function which will send back packets when they're completed
 */
void initReconstruction(void (*callback)(payload_t completed));

/** 
 * Adding a new chunk of data
 * 
 * @param data 
 */
void addChunk(payload_t data);


/** 
 * @param seq_no sequential number of the packet
 * 
 * @return pointer to the chunks
 */
stream_t *getChunks(int seq_no);

#endif
