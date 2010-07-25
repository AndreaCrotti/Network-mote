// FIXME: check if this is really working as expected

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "chunker.h"
#include "reconstruct.h"
#include "util.h"
#include "structs.h"

#define MSG_SIZE (1 << 10)

void read_device();

int main() {
    int fd = open("/dev/urandom", O_RDONLY);
    stream_t buff[MSG_SIZE];
    int nread = read(fd, buff, MSG_SIZE);
    
    // now we split the data and try to reconstruct it
    payload_t payload;
    payload.stream = (stream_t *) buff;
    payload.len = MSG_SIZE;
    
    ipv6Packet p[100];
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
    
    //TODO: use callback
    initReconstruction(0);
    printf("sizeof %d\n", sizeof(myPacketHeader) + MAX_CARRIED);

    for (int i = 0; i < count; i++) {
        // FIXME: addChunk((void *) &(p[i]));
        /* printf("%d, %d\n", get_seq_no(&p[i]), get_ord_no(&p[i])); */
    }
    
    // check if we got back the right data
    stream_t *chunks = getChunks(seq_no);
    for (int i = 0; i < MSG_SIZE; i++) {
        /* printf("%d == %d\n", chunks[i], buff[i]); */
        assert(chunks[i] == buff[i]);
    }
    
    // supposing now we have the right array of packets there
    return 0;
}
