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
    payload_t fixed_payload;
    fixed_payload.stream = (stream_t *) buff;
    fixed_payload.len = MSG_SIZE;
    
    // we need 100 elements of payloads which 
    payload_t payloads[100];

    char chunks_left;
    int count = 0;
    int seq_no = 0;
    int chunks_no = needed_chunks(nread);
    do {
        payload_t *added = &(payloads[count]);
        added->stream = malloc(sizeof(ipv6Packet));
        chunks_left = genIpv6Packet(&fixed_payload, (ipv6Packet *) added->stream, &(added->len), seq_no, count);
        count++;
        
    } while (chunks_left);
    
    //TODO: use callback
    initReconstruction(NULL);
    printf("sizeof %d\n", sizeof(myPacketHeader) + MAX_CARRIED);

    for (int i = 0; i < count; i++) {
        addChunk(payloads[count]);
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
