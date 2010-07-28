#ifndef RECONSTRUCT_H
#define RECONSTRUCT_H

#include "structs.h"

// using a dividend of 256 to make much less likely that nasty things happen
#define MAX_RECONSTRUCTABLE 32

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

/** 
 * Prints some statistical information about how many packets were completed. 
 */
void printStatistics(void);

#endif
