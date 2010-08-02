#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "../shared/structs.h"
#include "compress.h"
#include "various.h"

#define SIZE 12
int size = 1 << SIZE;

int main() {
    // Create payload for original, compressed and decompressed data
    stream_t data_msg[size];
    stream_t result_msg[size];
    stream_t compr_msg[size];
    
    payload_t msg;
    payload_t result = {
        .stream = result_msg,
        .len = size
    };
    payload_t decompressed = {
        .stream = compr_msg,
        .len = size
    };

    init_compression();
    msg.stream = data_msg;
    gen_compressable_payload(&msg, size / 2);

    payload_compress(msg, &result);
    payload_decompress(result, &decompressed);
    print_gained(msg.len, result.len);
    
    assert(payloadEquals(msg, decompressed));
    close_compression();
    return 0;
}
