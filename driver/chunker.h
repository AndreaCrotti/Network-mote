#ifndef CHUNKER_H
#define CHUNKER_H
#include <stdint.h>
#include "util.h"

#include "structs.h"


int gen_packet(payload_t* const payload, my_packet* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number);

void gen_my_packets2(payload_t *const payload, payload_t *const result, int const seq_no, const unsigned parts);

unsigned needed_chunks(int data_size);

#endif
