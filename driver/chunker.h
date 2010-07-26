#ifndef __CHUNKER_H
#define __CHUNKER_H
#include <stdint.h>
#include "util.h"

//TODO: We need to find a better solution here...
#include "../shared/structs.h"


int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, unsigned* sendsize, int const seq_no, int const chunk_number);

unsigned neededChunks(int data_size);

#endif
