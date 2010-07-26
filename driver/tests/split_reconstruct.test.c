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

void get_random_msg(payload_t data, int size) {
    int fd = open("/dev/urandom", O_RDONLY);
    int nread = read(fd, (void *) data.stream, size);
    data.len = size;
    assert(nread == size);
}

void swap_msgs(int p1, int p2, payload_t *msgs) {
    payload_t tmp;
    tmp = msgs[p1];
    msgs[p1] = msgs[p2];
    msgs[p2] = tmp;
}

// add all those packets in random order
void add_random_order(payload_t *msgs, int count) {
    int pos;
    while (count) {
        pos = random() % count;
        /* pos = (int) (count * (random() / (RAND_MAX + 1.0))); */
        addChunk(msgs[pos]);
        swap_msgs(pos, count-1, msgs);
        count--;
    }
    assert(count == 0);
}


int main() {
    // now we split the data and try to reconstruct it
    payload_t fixed_payload;
    stream_t buff[MSG_SIZE];
    fixed_payload.stream = buff;
    fixed_payload.len = MSG_SIZE;
    
    get_random_msg(fixed_payload, MSG_SIZE);
    
    // we need 100 elements of payloads which 
    payload_t payloads[100];

    char chunks_left;
    int count = 0;
    int seq_no = 0;
    int parts = neededChunks(MSG_SIZE);
    do {
        payload_t *added = &(payloads[count]);
        added->stream = malloc(sizeof(ipv6Packet));
        chunks_left = genIpv6Packet(&fixed_payload, (ipv6Packet *) added->stream, &(added->len), seq_no, parts);
        count++;
        
    } while (chunks_left);
    
    //TODO: use callback
    initReconstruction(NULL);

    add_random_order(payloads, parts);

    
    // check if we got back the right data
    stream_t *chunks = getChunks(seq_no);
    for (int i = 0; i < MSG_SIZE; i++) {
        assert(chunks[i] == buff[i]);
    }
    
    // supposing now we have the right array of packets there
    return 0;
}
