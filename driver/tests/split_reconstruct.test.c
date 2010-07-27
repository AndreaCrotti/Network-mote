// FIXME: check if this is really working as expected

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "chunker.h"
#include "reconstruct.h"
#include "util.h"
#include "structs.h"

int msg_size;
int num_msgs;

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

/** 
 * Set in result the mixing of all possible packets generated
 * 
 * @param fixed paylaod to add
 * @param result pointer to the result, allocated by the caller
 * @param parts number of parts needed for each
 * @param count how many times we want to add it
 */
void add_random_seqs(payload_t fixed, payload_t *result, int parts, int count) {
    // only one big array for all of them
    for (int seq = 0; seq < count; seq++) {
        payload_t copied = fixed;

        for (int i = 0; i < parts; i++) {
            int pos = (seq * parts) + i;
            payload_t *added = &(result[pos]);
            /* printf("adding on %d\n", seq * i); */
            (added->stream) = malloc(sizeof(ipv6Packet));
            // FIXME: there must a problem here since we are not setting the stream
            genPacket(&copied, (ipv6Packet *) added->stream, &(added->len), seq, parts);
            // add the chunks in random order now
        }
    }
    // now we have created the big array containing everything
    add_random_order(result, count * parts);
}

// return an array of ipv6Packet put in nice payload_t structures
// careful here
payload_t *gen_ipv6_payloads(int count) {
    payload_t *payloads = calloc(count, sizeof(payload_t));
    for (int i = 0; i < count; i++) {
        payloads[i].stream = malloc(sizeof(ipv6Packet));
        payloads[i].len = 0;
    }
    return payloads;
}

void free_payloads(payload_t *payloads, int count) {
    for (int i = 0; i < count; i++) {
        free((void *) payloads[i].stream);
    }
}

void simple_test(payload_t fixed) {
    int len = fixed.len;
    int chunks_no = neededChunks(len);
    printf("having %d\n", chunks_no);
    payload_t *result = gen_ipv6_payloads(chunks_no);
    
    genIpv6Packets2(&fixed, result, 0, chunks_no);

    initReconstruction(NULL);

    for (int i = 0; i < chunks_no; i++) {
        addChunk(result[i]);
    }
    
    stream_t *chunks = getChunks(0);

    for (int i = 0; i < msg_size; i++) {
        printf("i = %d, %d %d\n", i, chunks[i], fixed.stream[i]);
        /* assert(chunks[i] == fixed.stream[i]); */
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("usage: split_reconstruct <exp> <num msgs>\n");
        exit(1);
    }
    
    // now we split the data and try to reconstruct it
    msg_size = (1 << atoi(argv[1]));
    num_msgs = atoi(argv[2]);
    
    payload_t fixed_payload;
    stream_t buff[msg_size];
    fixed_payload.stream = buff;
    fixed_payload.len = msg_size;

    get_random_msg(fixed_payload, msg_size);
    /* simple_test(fixed_payload); */
    initReconstruction(NULL);

    int parts = neededChunks(msg_size);
    payload_t result[parts * num_msgs];

    add_random_seqs(fixed_payload, result, parts, num_msgs);

    // check if we got back the right data
    stream_t *chunks;
    for (int seq = 0; seq < num_msgs; seq++) {
        chunks = getChunks(seq);
        assert(chunks != NULL);
        // checking that the original data is the same as the data we compute
        for (int i = 0; i < msg_size; i++) {
            assert(chunks[i] == buff[i]);
        }
    }
    return 0;
}
