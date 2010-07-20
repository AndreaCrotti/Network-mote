#ifndef __CHUNKER_H
#define __CHUNKER_H
#include <stdint.h>
#include "util.h"

//TODO: We need to find a better solution here...
#include "../shared/structs.h"


/**
 * Generates an array of ipv6Packet
 * 
 * @param void * pointer to the data
 * @param int number of chunks (must be computed externally)
 * @param int sequential number of this packet
 * 
 * @return 
 */
int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, unsigned* sendsize, int const seq_no);

#endif
