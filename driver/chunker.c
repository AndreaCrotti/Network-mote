// -*- compile-command: "gcc -Wall chunker.c -o chunker" -*-
#include <stdlib.h>
#include <netinet/ip6.h>
#include <stdint.h>

// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED 127

typedef union {
    int len;
    uint8_t seq_no;
    uint8_t ordering_no;
} myPacket;

typedef union {
    struct ip6_hdr ip6_hdr;
    void *payload;
} ipv6Packet;

int main(int argc, char *argv) {
    
    return 0;
}

/*********************************************************************************************/
/* struct packet_header_s {                                                                  */
/*     unsigned len:32;                                                                      */
/*     unsigned seq_no:32;                                                                   */
/*     unsigned checksum:32;                                                                 */
/* } __attribute__((__packed__));                                                            */
/* typedef struct packet_header_s t_packet_header_s;                                         */
/* #define PACKET_HEADER_SIZE (sizeof(t_packet_header_s))                                    */
/*                                                                                           */
/* typedef struct packet_s {                                                                 */
/*     void *payload;                                                                        */
/*     t_packet_header_s head;                                                               */
/* } t_packet_s;                                                                             */
/*                                                                                           */
/* #define MAX_PAYLOAD_SIZE 50                                                               */
/* #define MAX_PAYLOAD_PP (MAX_PAYLOAD_SIZE-PACKET_HEADER_SIZE)                              */
/*                                                                                           */
/* int split_into_packets(void *data, size_T datasize, t_packet_s                            */
/*                        **outpackets, int *n_outpackets)                                   */
/* {                                                                                         */
/*     int i, sent;                                                                          */
/*     /\* calculate number of packets *\/                                                   */
/*     *n_outpackets = datasize/MAX_PAYLOAD_PP ;                                             */
/*     if (datasize%MAX_PAYLOAD_PP) ++*n_outpackets;                                         */
/*     /\* allocate outpackets *\/                                                           */
/*     if ((*outpackets =                                                                    */
/*          malloc(sizeof(***outpackets)*(*n_outpackets)))==NULL) /\* deal with err          */
/*                                                                 *\/                       */
/*         for (i=sent=0; i<*n_outpackets; ++i) {                                            */
/*             /\* allocate packets and copy data *\/                                        */
/*             if (((*outpackets)[i] =                                                       */
/*                  malloc(sizeof(*(*outpackets[i]))))==NULL) /\* deal with err *\/          */
/*                 (*outpackets)[i].payload = ((unsigned char *)data)+sent;                  */
/*             (*outpackets)[i].head.len = ((datasize-sent)>MAX_PAYLOAD_PP)?                 */
/*                 MAX_PAYLOAD_PP:datasize-sent;                                             */
/*             (*outpackets)[i].head.len += PACKET_HEADER_SIZE; /\* if you                   */
/*                                                                 want the head len too *\/ */
/*             (*outpackets)[i].head.seq_no = i;                                             */
/*             (*outpackets)[i].head.checksum = /\* whatever *\/                             */
/*                 sent += 50-PACKET_HEADER_SIZE;                                            */
/*         }                                                                                 */
/*     return 0;                                                                             */
/* }                                                                                         */
/*********************************************************************************************/
