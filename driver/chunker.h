#ifndef CHUNKER_H
#define CHUNKER_H
#include <stdint.h>
#include "util.h"

#include "structs.h"

/** 
 * Generates chunks on demand, keeping track of the status using static variables.
 * To use it cycle on the chunks_left, for example
 *   char chunks_left;
 *   do {
 *       chunks_left = gen_packet(&payload, &pkt, &sendsize, seqno, no_chunks);
 *       ...
 *   } while (chunks_left);
 *
 *
 * @param payload data to chunk
 * @param packet pointer to the memory where to store the chunk
 * @param sendsize size of the chunk
 * @param seq_no sequential number of the chunk
 * @param chunk_number ord number of the chunk
 *
 * @return number of chunks needed to finish
 */
int gen_packet(payload_t* const payload, my_packet* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number);

/** 
 * @param data_size data size of the chunk
 *
 * @return needed chunks for the given data size
 */
unsigned needed_chunks(int data_size);

void gen_my_packets2(payload_t *const payload, payload_t *const result, int const seq_no, const unsigned parts);

#endif
