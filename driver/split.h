#ifndef __SPLIT_H
#define __SPLIT_H


#define LEN 100
#define N_BYTES 8
typedef unsigned short csum_type;

typedef struct myPacket {
    void *packet;
    int len;
    uint8_t seq_no;
    uint8_t ordering_no;
    csum_type checksum;
} myPacket;

void **split_binary(const void *, int, int);
void *rebuild(void **, int, int);
csum_type csum(unsigned short *, int);
void print_packet(myPacket *, void (*my_print)(void *));
myPacket **gen_packets(void *, int, int);
void int_print(void *);


#endif
