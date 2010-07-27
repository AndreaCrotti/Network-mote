#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "../shared/structs.h"

// or we pass the pointer or we use *len and modify the pointer of the number
void getRandomMsg(payload_t *data, int size) {
    int fd = open("/dev/urandom", O_RDONLY);
    int nread = read(fd, (void *) data->stream, size);
    data->len = size;
    assert(nread == size);
}

void genCompressablePayload(payload_t *data, int size) {
    char string[] = "ciao ciao\0";
    data->len = size;
}
