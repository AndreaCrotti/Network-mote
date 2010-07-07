// -*- compile-command: "gcc -Wall chunker.c -o chunker" -*-
#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip6.h>
#include <stdint.h>
#include <string.h>

// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED 127


typedef struct in6_addr in6_src;
typedef struct in6_addr in6_dst;

// also the internal struct should be packed
struct myPacketHeader {
    uint8_t seq_no;
    uint8_t ord_no;
} __attribute__((__packed__));

typedef struct myPacketHeader myPacketHeader;

// only the final ipv6 packet must be "__packed__".
struct ipv6Packet {
    struct ip6_hdr ip6_hdr;
    myPacketHeader packetHeader;
    void *payload;
} __attribute__((__packed__));

// in this way making a simple cast on the ipV6 data I get the rest
typedef struct innerPacket {
    myPacketHeader packetHeader;
    void *payload;
} innerPacket;

typedef struct ipv6Packet ipv6Packet;

// TODO: is this the best way to solve this problem?
int MAX_PAYLOAD_SIZE = MAX_CARRIED - sizeof(myPacketHeader) - sizeof(struct ip6_hdr);

ipv6Packet **genIpv6Packets(void *data, int len, int seq_no);


int main(int argc, char **argv) {
    ipv6Packet v6;
    printf("v6 = %ld\n", sizeof(v6));
    int arr[10];
    printf("allocating some payload\n");
    int i;
    for (i = 0; i < 10; i++) {
        arr[i] = i;
    }
    v6.payload = arr;
    // now we try to send the packet and see if it's sniffable
    char *bigstring = "sdfklsjflksdjflksjdfkjsdfkjsdhfkjsdfhksdjh";
    
    ipv6Packet **packets = genIpv6Packets(bigstring, strlen(bigstring), 0);
    return 0;
}

/** 
 * Generate an array of ipv6Packet ready to send over the network
 * 
 * @param data data to split and encapsulate in chunks
 * @param len dimension in bytes of the data
 * 
 * @return 
 */
ipv6Packet **genIpv6Packets(void *data, int len, int seq_no) {
    int ord_no;
    int num_chunks = ((len + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE);
    // generating an array of ipv6Packet of the correct length
    ipv6Packet **buffer = calloc(num_chunks, sizeof(ipv6Packet));
    
    // the payload is copied in another side
    for (ord_no = 0; ord_no < num_chunks; ord_no++) {
        myPacketHeader *inner = &(buffer[ord_no]->packetHeader);
        inner->ord_no = ord_no;
        inner->seq_no = seq_no;
        buffer[ord_no]->payload = calloc(1, MAX_PAYLOAD_SIZE);
        // copying the data in the actual payload
        memcpy(buffer[ord_no]->payload, data, MAX_PAYLOAD_SIZE);
        // TODO: are we sure we want to create the same packet every time?
        
        // Check if going forward of bytes or what
        data += MAX_PAYLOAD_SIZE;
    }
    return buffer;
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

/* // Struttura dell' header di paccketto (fissa) */
/* typedef struct myPacket { */
/*      int len;   // Length of payload */
/*      unsigned int seq_no; */
/*      csum_type checksum; */
/* } myPacket_hdr; */


/* // Struttura generica del pacchetto -variabile :-) */
/* typedef struct packet{ */
/*        myPacket_hdr  hd; */
/*        int           dati[1]; // Trick: allocato a run time... */
/* // */
/* // NB: se il payload puo' essere differente */
/* //     union{ */
/* //        uint8_t  d8[1];    // payload dati 8bit */
/* //        uint16_t d16[1];   // payload dati 16bit */
/* //        uint32_t d32[1];   // payload dati 32bit */
/* //     }dati; */
/* }MP_T; */


/* // */
/* // Crea un nuovo pacchetto dati. */
/* // La lunghezza del pacchetto */
/* MP_T* create_packet(int seq, int numOfData, int* pPayBuf) */
/* { */
/*    MP_T*  pPack; */
/*    size_t len; */

/*    len= (sizeof(int)*numOfData); */

/*    if(NULL != (pPack = malloc(len+sizeof(myPacket_hdr)))) */
/*    { */
/*         pPack->hd.len=len; */
/* 	pPack->hd.seq_no=seq; */

/*         if((NULL != pPayBuf) && (len)) */
/*         { */
/*           // Se il payload NON e' vuoto, copialo!!! */
/*            memcpy((void*)&pPack->dati[0], (void*)pPayBuf, len); */
/*         } */
/*         // calcola checksum.... */
/* 	pPack->hd.checksum=compute_checksum(pPack); */
/*    } */
/*    return(pPack);		 */
/* } */
