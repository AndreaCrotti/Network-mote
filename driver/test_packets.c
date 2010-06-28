// -*- compile-command: "gcc -Wall -Wextra -lm -o test test_packets.c" -*-
// check out XDR for data binary manipulation also
#include <stdlib.h>
#include <pcap.h>
#include <string.h>
#include <math.h>

#define SIZE 100

void **split_binary(const void *, int, int);
void *rebuild(void **, int, int);

int main() {
    // create some data and split and recombine it
    int i;
    int data[SIZE];
    for (i = 0; i < SIZE; i++) {
        data[i] = i;
    }
    void **chunks = split_binary(data, SIZE, 3);
    // now check if they're equivalent
    int *reconstructed = rebuild(chunks, 34, SIZE);
    // use assert to check for equivalence
    for (i = 0; i < SIZE; i++) {
        printf("%d", (int) reconstructed[i]);
    }
    return(0);
}


// splitting the data in binary chunks of len len, returning
// a pointer to the array of chunks
void **split_binary(const void *data, int size, int chunk_size) {
    // allocate enough pointers here, make sure "int" is doing the ceil!
    int chunks = (ceil) (size / chunk_size);
    void **result = malloc(sizeof(void *) * chunks);
    int i;
    for (i = 0; i < size; i++) {
        result[i] = malloc(sizeof(void *) * chunk_size);
        memcpy(result[i], data + (sizeof(void *) * chunk_size), chunk_size);
    }
    printf("splitting object of length %d in %d pieces\n", size, chunks);
    return result;
}

// reconstruct the object from the chunks and the original size
void *rebuild(void **chunks, int len, int size) {
    int i;
    void *data = malloc(size);
    for (i = 0; i < len; i++) {
        data += sizeof(chunks[i]);
        memcpy(data, chunks[i], sizeof(chunks[i]));
    }
    return data;
}
