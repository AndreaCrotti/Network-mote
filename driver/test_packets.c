// -*- compile-command: "gcc -Wall -Wextra -lm -lpcap -o test test_packets.c && ./test" -*-
// TODO: add a checksum field
// TODO: try with real IPv4 packets and try to send them over the localhost interface

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <pcap.h>
/* #include <netinet/ip6.h> */

#define LEN 100
#define N_BYTES 8

void **split_binary(const void *, int);
void *rebuild(void **, int, int);

int main() {
    // create some data and split and recombine it
    int i;
    int data[LEN];
    for (i = 0; i < LEN; i++) {
        data[i] = i;
    }
    int nbytes = LEN * sizeof(int);
    void **chunks = split_binary(data, N_BYTES);
    // now check if they're equivalent
    int *reconstructed = (int *) rebuild(chunks, nbytes, N_BYTES);
    // use assert to check for equivalence
    for (i = 0; i < LEN; i++) {
        /* printf("data[i] = %d, reconstructed[i] = %d\n", data[i], reconstructed[i]); */
        assert(data[i] == (int) reconstructed[i]);
    }
    // now we should free them also
    free(reconstructed);
    free(chunks);
    return(0);
}


// splitting the data in binary chunks of len len, returning
// a pointer to the array of chunks
void **split_binary(const void *data, int chunk_size) {
    // allocate enough pointers here, make sure "int" is doing the ceil!
    int i;
    int num_chunks = (ceil) ((float) sizeof(data) * LEN / chunk_size);
    void **result = malloc(sizeof(void *) * num_chunks);
    printf("allocating %d times %d bytes and tot = %ld\n", num_chunks, chunk_size, sizeof(data)*LEN);

    for (i = 0; i < num_chunks; i++) {
        result[i] = calloc(1, chunk_size);
        memcpy(result[i], data + (i * chunk_size), chunk_size);
    }
    return result;
}

void *rebuild(void **chunks, int size, int chunk_size) {
    void *data = malloc(size);
    int num_chunks = (ceil) ((float) size / chunk_size);
    int i;
    for (i = 0; i < num_chunks; i++) {
        memcpy(data + (i * chunk_size), chunks[i], chunk_size) ;
        /* printf("setting %d bytes at offset %d\n", chunk_size, (i * chunk_size)); */
    }
    return data;
}
