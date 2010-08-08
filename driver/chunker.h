#ifndef CHUNKER_H
#define CHUNKER_H
#include <stdint.h>
#include "util.h"

//TODO: We need to find a better solution here...
#include "structs.h"


int gen_packet(payload_t* const payload, myPacket* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number);

void gen_myPackets2(payload_t *const payload, payload_t *const result, int const seq_no, const unsigned parts);

unsigned needed_chunks(int data_size);

#endif
