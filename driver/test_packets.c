// -*- compile-command: "gcc -Wall -Wextra -lm -lpcap -o test test_packets.c && ./test" -*-
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include <pcap.h>

#define LEN 100
#define N_CHUNKS 400

void **split_binary(const void *, int, int);
void *rebuild(void **, int, int);

int main() {
    // create some data and split and recombine it
    int i;
    int data[LEN];
    for (i = 0; i < LEN; i++) {
        data[i] = i;
    }
    int nbytes = LEN * sizeof(int);
    void **chunks = split_binary(data, nbytes, N_CHUNKS);
    // now check if they're equivalent
    int *reconstructed = (int *) rebuild(chunks, nbytes, N_CHUNKS);
    // use assert to check for equivalence
    for (i = 0; i < LEN; i++) {
        /* printf("data[i] = %d, reconstructed[i] = %d\n", data[i], reconstructed[i]); */
        assert(data[i] == (int) reconstructed[i]);
    }
    return(0);
}


// splitting the data in binary chunks of len len, returning
// a pointer to the array of chunks
void **split_binary(const void *data, int size, int num_chunks) {
    // allocate enough pointers here, make sure "int" is doing the ceil!
    int chunk_size = (ceil) ((float) size / num_chunks);
    int i;
    void **result = malloc(sizeof(void *) * num_chunks);
    printf("allocating %d times %d bytes\n", num_chunks, chunk_size);

    for (i = 0; i < num_chunks; i++) {
        result[i] = calloc(1, chunk_size);
        memcpy(result[i], data + (i * chunk_size), chunk_size);
    }
    return result;
}

void *rebuild(void **chunks, int size, int num_chunks) {
    void *data = malloc(size);
    int chunk_size = (ceil) ((float) size / num_chunks);
    int i;
    for (i = 0; i < num_chunks; i++) {
        memcpy(data + (i * chunk_size), chunks[i], chunk_size) ;
        /* printf("setting %d bytes at offset %d\n", chunk_size, (i * chunk_size)); */
    }
    return data;
}
