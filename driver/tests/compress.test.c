// testing the compressor/decompressor

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>

#include "../shared/structs.h"
#include "../compress.h"

// a typical dimension we might try
#define SIZE 10
int size = 1 << SIZE;

void get_random_msg(payload_t data, int size) {
    int fd = open("/dev/urandom", O_RDONLY);
    int nread = read(fd, (void *) data.stream, size);
    data.len = size;
    assert(nread == size);
}

int payload_equals(payload_t x, payload_t y) {
    if (x.len != y.len)
        return 0;
       
    for (unsigned i = 0; i < x.len; i++) {
        if (x.stream[i] != y.stream[i])
            return 0;
    }
    return 1;
}

int main() {
    stream_t data_msg[size];
    stream_t result_msg[size];
    stream_t compr_msg[size];
    
    payload_t msg;
    payload_t result;
    payload_t decompressed;

    msg.stream = data_msg;
    result.stream = result_msg;
    decompressed.stream = compr_msg;

    get_random_msg(msg, size);
    
    // now try to compress the data and see what happens
    payload_compress(msg, result);

    payload_decompress(result, decompressed);
    
    printf("now checking if decompressed is the original data\n");
    assert(payload_equals(msg, decompressed));

    // now try to decompress the data and see if it's equal
    return 0;
}
