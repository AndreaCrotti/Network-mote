#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "structs.h"

// or we pass the pointer or we use *len and modify the pointer of the number
void get_random_msg(payload_t *data, int size) {
    int fd = open("/dev/urandom", O_RDONLY);
    int nread = read(fd, (void *) data->stream, size);
    data->len = size;
    assert(nread == size);
}

void gen_compressable_payload(payload_t *data, int size) {
    memset((void *)data->stream, 0xA, size);
    data->len = size;
}
