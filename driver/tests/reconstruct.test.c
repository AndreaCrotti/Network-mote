#include <stdlib.h>
#include <stdio.h>

#include "reconstruct.h"
#include "util.h"
#include "structs.h"

int num_packets = 10;
void test_addressing();
void test_last();

void (*callback)(payload_t completed);

void fake_callback(payload_t completed) { (void) completed;}

// doing some simple testing
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    // give it a real function
    /* initReconstruction(fake_callback); */
    /* ipv6Packet *pkt = calloc(num_packets, sizeof(ipv6Packet)); */
    
    /* // create some fake payload to create correctly the payload */
    
    /* test_last(); */
    /* stream_t x[2] = {0, 1}; */

    /* for (int i = 0; i < num_packets; i++) { */
    /*     makeIpv6Packet(&(pkt[i]), 0, i, num_packets, x, 2); */
    /*     /\* addChunk((void *) &pkt[i]); *\/ */
    /* } */

    /* get_packet(0); */
    /* // 0 for example can't be found */
    /* // assert(get_packet(0) == NULL);  */

    /* test_addressing(); */

    /* free(pkt); */
    return 0;
}
