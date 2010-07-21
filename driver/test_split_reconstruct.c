#ifdef STANDALONE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "chunker.h"
#include "reconstruct.h"
#include "util.h"
#include "structs.h"
#include "variables.h"

#define MSG_SIZE (1 << 10)

void read_device();

int main(int argc, char *argv[]) {
    int fd = open("/dev/urandom", O_RDONLY);
    char *buff = malloc(sizeof(char)  * MSG_SIZE);
    int nread = read(fd, buff, MSG_SIZE);
    printf("read %d bytes from random\n", nread);
    
    // now we split the data and try to reconstruct it
    payload_t payload;
    payload.stream = (stream_t *) buff;
    payload.len = MSG_SIZE;
    
    payload_t * const pointer = &payload;
    
    ipv6Packet *p = malloc(sizeof(ipv6Packet) * 10);
    // now send the message
    /* unsigned *send_size = malloc(sizeof(unsigned)); */
    /* int x = genIpv6Packet(&payload, p, send_size, 0); */

    /* printf("splitting the packets we've got %x data \n", x); */

    // supposing now we have the right array of packets there
    read_device();
    
    int x = getFd();
    setFd(10);
    
    printf("before %d now %d\n", x, getFd());
    setFd(3);
    return 0;
}

void read_device() {
    char buf[10];
    FILE *fp = fopen("curdev", "r");
    int read = fscanf(fp, "%s", buf);
    printf("read %d chars and string %s\n", read, buf);

    FILE *wp = fopen("curdev2", "w");
    // we can use also fwrite to write out
}

#endif
