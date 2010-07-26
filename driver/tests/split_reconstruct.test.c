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

void add_random_seqs(payload_t fixed, payload_t *result, int parts, int count) {
    // only one big array for all of them
    printf("parts, seq = %d, %d\n", parts, count);
    for (int seq = 0; seq < count; seq++) {
        for (int i = 0; i < parts; i++) {
            payload_t *added = &(result[seq * i]);
            printf("adding on %d\n", seq * i);
            (added->stream) = malloc(sizeof(ipv6Packet));
            genIpv6Packet(&fixed, (ipv6Packet *) added->stream, &(added->len), seq, parts);
            printf("added %d\n", added->len);
            // add the chunks in random order now
        }
        add_random_order(result + (seq * parts), parts);
    }
    printf("calling on seq %d\n", (parts * count) -1);
}

int main() {
    // now we split the data and try to reconstruct it
    payload_t fixed_payload;
    stream_t buff[MSG_SIZE];
    fixed_payload.stream = buff;
    fixed_payload.len = MSG_SIZE;
    
    get_random_msg(fixed_payload, MSG_SIZE);
    
    initReconstruction(NULL);

    int num_msgs = 1;

    int parts = neededChunks(MSG_SIZE);
    payload_t result[parts * (num_msgs + 1)];

    add_random_seqs(fixed_payload, result, parts, num_msgs);

    
    // check if we got back the right data
    stream_t *chunks;
    for (int seq = 0; seq < num_msgs; seq++) {
        chunks = getChunks(seq);
        for (int i = 0; i < MSG_SIZE; i++) {
            assert(chunks[i] == *(result[i * seq].stream));
        }
    }
    return 0;
}
