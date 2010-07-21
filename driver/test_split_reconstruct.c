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
    stream_t *buff[MSG_SIZE];
    int nread = read(fd, buff, MSG_SIZE);
    printf("read %d bytes from random\n", nread);
    
    // now we split the data and try to reconstruct it
    payload_t payload;
    payload.stream = (stream_t *) buff;
    payload.len = MSG_SIZE;
    
    payload_t *pointer = &payload;
    
    ipv6Packet p[20];
    // now send the message
    unsigned send_size;

    char chunks_left;
    int count = 0;
    int seq_no = 0;
    int chunks_no = needed_chunks(nread);
    do {
        chunks_left = genIpv6Packet(&payload, &(p[count]), &send_size, seq_no, chunks_no);
        count++;
    } while (chunks_left);
    
    initReconstruction();
    printf("sizeof %d\n", sizeof(myPacketHeader) + MAX_CARRIED);

    for (int i = 0; i < count; i++) {
        addChunk((void *) &(p[i]));
        /* printf("%d, %d\n", get_seq_no(&p[i]), get_ord_no(&p[i])); */
    }
    
    // check if we got back the right data
    stream_t *chunks = getChunks(seq_no);
    for (int i = 0; i < MSG_SIZE; i++) {
        printf("%d == %d\n", chunks[i], buff[i]);
        assert(chunks[i] == buff[i]);
    }
    
    // supposing now we have the right array of packets there
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
