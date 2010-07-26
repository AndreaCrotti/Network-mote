#ifndef __CHUNKER_H
#define __CHUNKER_H
#include <stdint.h>
#include "util.h"

//TODO: We need to find a better solution here...
#include "../shared/structs.h"


int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number);

/* void genIpv6Packets2(payload_t *const payload, ipv6Packet *const result, int const seq_no, const int parts); */

unsigned neededChunks(int data_size);

#endif
