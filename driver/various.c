#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "../shared/structs.h"

void getRandomMsg(payload_t data, int size) {
    int fd = open("/dev/urandom", O_RDONLY);
    int nread = read(fd, (void *) data.stream, size);
    data.len = size;
    assert(nread == size);
}
