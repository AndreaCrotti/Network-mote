// -*- compile-command: "gcc -Wall chunker.c -o chunker" -*-
#include <stdlib.h>
#include <netinet/ip6.h>
#include <stdint.h>

// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED 127

struct myPacket {
    int len;
    uint8_t seq_no;
    uint8_t ordering_no;
    int payload[32];
}; //__attribute__((__packed__));

typedef struct myPacket myPacket;

struct ipv6Packet {
    struct ip6_hdr ip6_hdr;
    myPacket payload;
}; // __attribute__((__packed__));

typedef struct ipv6Packet ipv6Packet;

int main(int argc, char *argv) {
    ipv6Packet v6;
    printf ("size = %d, int %d, uint8_t * 2 (%d), 32, ipv6header (%d)\n", sizeof(v6), sizeof(int), sizeof(uint8_t), sizeof(struct ip6_hdr));
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

// Struttura dell' header di paccketto (fissa)
typedef struct myPacket {
     int len;   // Length of payload
     unsigned int seq_no;
     csum_type checksum;
} myPacket_hdr;


// Struttura generica del pacchetto -variabile :-)
typedef struct packet{
       myPacket_hdr  hd;
       int           dati[1]; // Trick: allocato a run time...
//
// NB: se il payload puo' essere differente
//     union{
//        uint8_t  d8[1];    // payload dati 8bit
//        uint16_t d16[1];   // payload dati 16bit
//        uint32_t d32[1];   // payload dati 32bit
//     }dati;
}MP_T;


//
// Crea un nuovo pacchetto dati.
// La lunghezza del pacchetto
MP_T* create_packet(int seq, int numOfData, int* pPayBuf)
{
   MP_T*  pPack;
   size_t len;

   len= (sizeof(int)*numOfData);

   if(NULL != (pPack = malloc(len+sizeof(myPacket_hdr))))
   {
        pPack->hd.len=len;
	pPack->hd.seq_no=seq;

        if((NULL != pPayBuf) && (len))
        {
          // Se il payload NON e' vuoto, copialo!!!
           memcpy((void*)&pPack->dati[0], (void*)pPayBuf, len);
        }
        // calcola checksum....
	pPack->hd.checksum=compute_checksum(pPack);
   }
   return(pPack);		
}
