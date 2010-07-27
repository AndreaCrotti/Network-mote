// testing the compressor/decompressor

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "../shared/structs.h"
#include "../compress.h"
#include "../various.h"

// a typical dimension we might try
#define SIZE 10
int size = 1 << SIZE;

/* const char *mylongstring = "ciao ciao very very likely to compress\n\0"; */
/* int size = strlen(mylongstring); */

int main() {
    stream_t data_msg[size];
    stream_t result_msg[size * 2];
    stream_t compr_msg[size * 2];
    
    payload_t msg;
    payload_t result;
    payload_t decompressed;

    msg.stream = data_msg;
    result.stream = result_msg;
    decompressed.stream = compr_msg;
    
    /* data_msg = mylongstring; */
    getRandomMsg(&msg, size);
    
    // now try to compress the data and see what happens
    payload_compress(msg, &result);
    payload_decompress(result, &decompressed);
    
    printf("now checking if decompressed is the original data\n");
    assert(payloadEquals(msg, decompressed));

    // now try to decompress the data and see if it's equal
    return 0;
}
