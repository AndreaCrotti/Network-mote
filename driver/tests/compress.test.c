#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "structs.h"
#include "compress.h"

#define SIZE 12
int size = 1 << SIZE;

void get_random_msg(payload_t *data, int size) {
    int fd = open("/dev/urandom", O_RDONLY);
    int nread = read(fd, (void *) data->stream, size);
    data->len = size;
    assert(nread == size);
}

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
    get_random_msg(&msg, size / 2);

    for (int i = 0; i < 100; i++) {
        payload_compress(msg, &result);
        payload_decompress(result, &decompressed);
        print_gained(msg.len, result.len);
        assert(payload_equals(msg, decompressed));
    }

    close_compression();
    return 0;
}
