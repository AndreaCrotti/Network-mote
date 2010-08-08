#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "chunker.h"
#include "reconstruct.h"
#include "util.h"
#include "structs.h"
#include "various.h"

// this is the exponent of the size
#define MSG_SIZE 10
#define NUM_MSGS 5

int msg_size;
int num_msgs;

/** 
 * Swap two messages at the given positions
 * 
 * @param p1 
 * @param p2 
 * @param msgs 
 */
void swap_msgs(int p1, int p2, payload_t *msgs) {
    payload_t tmp;
    tmp = msgs[p1];
    msgs[p1] = msgs[p2];
    msgs[p2] = tmp;
}

void add_random_order(payload_t *msgs, int count) {
    int pos;
    while (count) {
        pos = random() % count;
        add_chunk(msgs[pos]);
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
            (added->stream) = malloc(sizeof(myPacket));
            gen_packet(&copied, (myPacket *) added->stream, &(added->len), seq, parts);
        }
    }
    // now we have created the big array containing everything
    add_random_order(result, count * parts);
}

int main(int argc, char *argv[]) {
    if ((argc != 3) && (argc != 1)) {
        printf("usage: split_reconstruct [exp] [num msgs]\n");
    }
    if (argc == 3) {
        // now we split the data and try to reconstruct it
        msg_size = (1 << atoi(argv[1]));
        num_msgs = atoi(argv[2]);
    } else if (argc == 1) {
        msg_size = MSG_SIZE;
        num_msgs = NUM_MSGS;
    }
    
    payload_t fixed_payload;
    stream_t buff[msg_size];
    fixed_payload.stream = buff;
    fixed_payload.len = msg_size;

    get_random_msg(&fixed_payload, msg_size);
    /* simple_test(fixed_payload); */
    init_reconstruction(NULL);

    int parts = needed_chunks(msg_size);
    payload_t result[parts * num_msgs];

    add_random_seqs(fixed_payload, result, parts, num_msgs);

    // check if we got back the right data
    stream_t *chunks;
    for (int seq = 0; seq < num_msgs; seq++) {
        chunks = get_chunks(seq);
        assert(chunks != NULL);
        // checking that the original data is the same as the data we compute
        for (int i = 0; i < msg_size; i++) {
            assert(chunks[i] == buff[i]);
        }
    }
    return 0;
}
