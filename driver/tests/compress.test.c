#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "../shared/structs.h"
#include "compress.h"
#include "various.h"

// a typical dimension we might try
#define SIZE 12
int size = 1 << SIZE;

int main() {
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

    msg.stream = data_msg;
    genCompressablePayload(&msg, size / 2);

    payloadCompress(msg, &result);
    payloadDecompress(result, &decompressed);
    printGained(msg.len, result.len);

    assert(payloadEquals(msg, decompressed));
    return 0;
}
