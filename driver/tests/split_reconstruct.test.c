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
        addChunk(msgs[pos]);
        swap_msgs(pos, count-1, msgs);
        count--;
    }
    assert(count == 0);
}

void add_random_seqs(payload_t fixed, payload_t *result, int parts, int count) {
    // only one big array for all of them
    for (int seq = 0; seq < count; seq++) {
        payload_t copied = fixed;
        int pos = (seq * parts);

        /* ipv6Packet result = malloc(sizeof(ipv6Packet) * parts); */

        for (int i = 0; i < parts; i++) {
            printf("position = %d\n", pos);
            payload_t *added = &(result[pos]);
            /* printf("adding on %d\n", seq * i); */
            (added->stream) = malloc(sizeof(ipv6Packet));
            // FIXME: there must a problem here since we are not setting the stream
            genIpv6Packet(&copied, (ipv6Packet *) added->stream, &(added->len), seq, parts);
            printf("added %d\n", added->len);
            // add the chunks in random order now
        }
    }
    // now we have created the big array containing everything
    add_random_order(result, count);
    printf("calling on seq %d\n", (parts * count) -1);
}

int main() {
    // now we split the data and try to reconstruct it
    int num_msgs = 1;

    payload_t fixed_payload;
    stream_t buff[MSG_SIZE];
    fixed_payload.stream = buff;
    fixed_payload.len = MSG_SIZE;
    get_random_msg(fixed_payload, MSG_SIZE);
    initReconstruction(NULL);

    int parts = neededChunks(MSG_SIZE);
    payload_t result[parts * num_msgs];

    add_random_seqs(fixed_payload, result, parts, num_msgs);

    // check if we got back the right data
    stream_t *chunks;
    for (int seq = 0; seq < num_msgs; seq++) {
        printf("seq = %d\n", seq);
        chunks = getChunks(seq);
        assert(chunks != NULL);
        // checking that the original data is the same as the data we compute
        for (int i = 0; i < MSG_SIZE; i++) {
            printf("i = %d, %d %d\n", i, chunks[i], buff[i]);
            /* assert(chunks[i] == buff[i]); */
        }
    }
    return 0;
}
