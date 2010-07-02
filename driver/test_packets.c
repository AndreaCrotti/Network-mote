// -*- compile-command: "gcc -Wall -Wextra -lm -lpcap -o test test_packets.c && ./test" -*-
// TODO: add a checksum field
// TODO: try with real IPv4 packets and try to send them over the localhost interface
// TODO: check if I'm freeing the memory correctly with
/**
 * @file   test_packets.c
 * @author Andrea Crotti <andrea.crotti.0@gmail.com>
 * @date   Fri Jul  2 16:24:18 2010
 * 
 * @brief  - Read data from the tun device (making sure we know the structure used)
 *         - Split the packet giving some sort of sequential number and a checksum
 *         - Send it over the channel including the payload there
 * 
 */


#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <pcap.h>

/* #include <netinet/ip6.h> */

#define LEN 100
#define N_BYTES 8
typedef unsigned short csum_type;

typedef struct myPacket {
    void *packet;
    int len;
    unsigned int seq_no;
    csum_type checksum;
} myPacket;

void **split_binary(const void *, int, int);
void *rebuild(void **, int, int);
csum_type csum(unsigned short *, int);
void print_packet(myPacket *, void (*my_print)(void *));
myPacket **gen_packets(void *, int, int);
void int_print(void *);
void send(uint8_t * stream, unsigned count);

int num_chunks;

void send(uint8_t* stream, unsigned count) {}

int main() {
    // create some data and split and recombine it
    int i;
    int data[LEN];
    for (i = 0; i < LEN; i++) {
        data[i] = i;
    }
    int nbytes = LEN * sizeof(int);
    num_chunks = ((nbytes + N_BYTES - 1) / N_BYTES);

    /*void **chunks = split_binary(data, nbytes, N_BYTES);
    // now check if they're equivalent
    int *reconstructed = (int *) rebuild(chunks, nbytes, N_BYTES);
    // use assert to check for equivalence
    for (i = 0; i < LEN; i++) {
        // printf("data[i] = %d, reconstructed[i] = %d\n", data[i], reconstructed[i]);
        assert(data[i] == (int) reconstructed[i]);
        //send(data[i],N_BYTES);
    }
    */
    // send the data
    uint8_t* p = (uint8_t*)data;
    for (i = 0; i < num_chunks; i++) {
      send(p,N_BYTES);
      p += N_BYTES;
    }
    /* // now we should free them also */
    
    /* free(reconstructed); */
    /* for (i = 0; i < num_chunks; i++) { */
    /*     free(chunks[i]); */
    /* } */
    /* free(chunks); */
    
    /* // FIXME: this should be a pointer to pointer instead */
    /* myPacket **packets = gen_packets(data, nbytes, 20); */
    /* for (i = 0; i < num_chunks; i++) { */
    /*     print_packet(packets[i], int_print); */
    /* } */
    return(0);
}

// generates all the packets given the data and the maximal carried bytes
myPacket **gen_packets(void *data, int nbytes, int mtu) {
    int i;
    int max_bytes = mtu - (sizeof(int) - sizeof(unsigned int) - sizeof(csum_type));
    // here setting it globally, very ugly
    num_chunks = (ceil) ((float) nbytes / mtu);
    // allocates first the array
    myPacket **packets = malloc(sizeof(myPacket *) * num_chunks);
    void **splitted = split_binary(data, nbytes, max_bytes);
    
    for (i = 0; i < num_chunks; i++) {
        packets[i] = malloc(sizeof(myPacket));
        // now only using an arrow or not?
        packets[i]->packet = splitted[i];
        packets[i]->seq_no = i;
        packets[i]->len = max_bytes;
        // compute also the checksum
    }
    return packets;
}

// we get an unordered data set and we want to rebuild it correctly
myPacket *reconstruct_data(myPacket **data, int num_chunks) {
    // we can use bin_search
    return NULL;
}

// takes a pointer to function to print correctly the internal data
void print_packet(myPacket *packet, void (*my_print)(void *data)) {
    my_print(packet->packet);
    printf("seq_no = %d\n", packet->seq_no);
}

// example of a print function
void int_print(void *data) {
    printf("%d\n", *((int *) data));
}

// Function for checksum calculation. From the RFC,
// the checksum algorithm is:
//  "The checksum field is the 16 bit one's complement of the one's
//  complement sum of all 16 bit words in the header.  For purposes of
//  computing the checksum, the value of the checksum field is zero."
csum_type csum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for(sum=0; nwords>0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}

// splitting the data in binary chunks of len len, returning
// a pointer to the array of chunks
void **split_binary(const void *data, int tot_size, int chunk_size) {
    // allocate enough pointers here, make sure "int" is doing the ceil!
    int i;
    /* void **result = malloc(sizeof(void *) * num_chunks); */
    void **result = calloc(num_chunks, chunk_size);
    printf("allocating %d times %d bytes and tot = %d\n", num_chunks, chunk_size, tot_size);

    for (i = 0; i < (num_chunks * chunk_size); i+=chunk_size) {
        /* result[i] = calloc(1, chunk_size); */
        memcpy(result[i], data + (i * chunk_size), chunk_size);
    }
    return result;
}

void *rebuild(void **chunks, int size, int chunk_size) {
    void *data = malloc(size);
    int i;
    for (i = 0; i < num_chunks; i++) {
        memcpy(data + (i * chunk_size), chunks[i], chunk_size) ;
        /* printf("setting %d bytes at offset %d\n", chunk_size, (i * chunk_size)); */
    }
    return data;
}
