#ifdef STANDALONE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "chunker.h"
#include "reconstruct.h"

#define MSG_SIZE (1 << 10)

int main(int argc, char *argv[]) {
    int fd = open("/dev/urandom", O_RDONLY);
    char *buff = malloc(sizeof(char)  * MSG_SIZE);
    int nread = read(fd, buff, MSG_SIZE);
    printf("read %d bytes from random\n", nread);
    
    // now we split the data and try to reconstruct it
    // how do I do this exactly

    return 0;
}

#endif

