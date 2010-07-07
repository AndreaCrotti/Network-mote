// -*- compile-command: "gcc -Wall chunker.c -o chunker" -*-
// TODO: add one End Of Packet packet which means end of transmission for packet with seq_no
// TODO: check integer division
// TODO: send the raw packets generated via the tun device
// TODO: use perror to print out errors when possible

#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip6.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED 127

typedef struct in6_addr in6_src;
typedef struct in6_addr in6_dst;

// also the internal struct should be packed
struct myPacketHeader {
    uint8_t seq_no;
    uint8_t ord_no;
    unsigned short checksum;
} __attribute__((__packed__));

typedef struct myPacketHeader myPacketHeader;

// only the final ipv6 packet must be "__packed__".
struct ipv6Packet {
    struct ip6_hdr ip6_hdr;
    myPacketHeader packetHeader;
    void *payload;
} __attribute__((__packed__));

typedef struct ipv6Packet ipv6Packet;

// just for more ease of writing
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct ip6_hdr ip6_hdr;

// in this way making a simple cast on the ipV6 data I get the rest
// NOT USING THIS at the moment
/* typedef struct innerPacket { */
/*     myPacketHeader packetHeader; */
/*     void *payload; */
/* } innerPacket; */


// TODO: is this the best way to solve this problem?
int MAX_PAYLOAD_SIZE = MAX_CARRIED - sizeof(myPacketHeader) - sizeof(struct ip6_hdr);

ipv6Packet *genIpv6Packets(void *data, int len, int seq_no);
void *reconstruct(ipv6Packet *, int);
unsigned short csum(unsigned short *, int);
void dataToLocalhost(void *, int, int);
ip6_hdr genIpv6Header(size_t);

int main(int argc, char **argv) {
    ipv6Packet v6;
    printf("v6 = %ld\n", sizeof(v6));
    // now we try to send the packet and see if it's sniffable
    int i;

    int *arr = calloc(sizeof(int), 1000);
    for (i = 0; i < 1000; i++) {
        arr[i] = i;
    }
    int num_chunks = (int)(1000 / MAX_PAYLOAD_SIZE) + 1;
    dataToLocalhost(arr, num_chunks, 0);
    return 0;
}

ip6_hdr genIpv6Header(size_t payload_len) {
    ip6_hdr header;
    printf("payload len = %ld\n", payload_len);
    header.ip6_src = in6addr_loopback;
    header.ip6_dst = in6addr_loopback;
    header.ip6_ctlun.ip6_un1.ip6_un1_plen = payload_len;
    /* header->ip6_src = 0; */
    return header;
}

sockaddr_in6 localhostDest() {
    sockaddr_in6 dest;
    dest.sin6_family = AF_INET6;
    // manual way to set the loopback interface
    /* for(i = 0; i < 7; i++) { */
    /*     dest.sin6_addr.s6_addr16[i] = 0; */
    /* } */
    dest.sin6_addr = in6addr_loopback;

    dest.sin6_addr.s6_addr16[7]=htons(1);
    dest.sin6_flowinfo = 0;
    dest.sin6_scope_id = 0;
    /* dest.sin6_port = htons(9999); */
    dest.sin6_port = 0;
    return dest;
}

// create some data and send it 
void dataToLocalhost(void *data, int num_chunks, int seq_no) {
    ipv6Packet *buffer = genIpv6Packets(data, num_chunks, seq_no);
    
    sockaddr_in6 dest = localhostDest();
    // send to localhost simply using a raw socket
    int raw_sock;
    
    if((raw_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket() error");
    }

    // now set up the correct fields to it
    
    
    // this is necessary for using raw packets
    /* if(setsockopt(raw_sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) { */
    /*     perror("cant setup the ip structure stuff"); */
    /* } */

    int i;
    for (i = 0; i < num_chunks; i++) {
        // actually sending away my data with given length
        if (sendto (raw_sock,
                    &buffer,
                    sizeof(ipv6Packet),
                    0,
                    (struct sockaddr *) &dest,
                    sizeof (dest)) < 0)  {
            printf ("Error in send\n");
            exit(1);
        } else {
            printf("packet %d sent correctly, sniff it\n", i);
        }
        buffer++;
    }
}

/** 
 * Generate an array of ipv6Packet ready to send over the network
 * 
 * @param data data to split and encapsulate in chunks
 * @param num_chunks number of chunks in which to split
 * 
 * @return 
 */
ipv6Packet *genIpv6Packets(void *data, int num_chunks, int seq_no) {
    int ord_no;
    // check this "integer" division
    // generating an array of ipv6Packet of the correct length
    ipv6Packet *buffer = calloc(num_chunks, sizeof(ipv6Packet));
    // copy this every time we need a new one
    ipv6Packet original;
    // passing the length of the payload
    original.ip6_hdr = genIpv6Header(sizeof(myPacketHeader) + MAX_PAYLOAD_SIZE);
    // set up some common fields
    myPacketHeader pkt_header = original.packetHeader;
    pkt_header.seq_no = seq_no;
    // setup here the ipv6 fields here

    // only the payload and the ord_no are changing
    for (ord_no = 0; ord_no < num_chunks; ord_no++) {
        // simply copying by value the original packet
        buffer[ord_no] = original;
        // should work anyway?
        /* memcpy(buffer[ord_no], &original, sizeof(ipv6Packet)); */
        printf("copied the memory correctly, ord_no = %d\n", ord_no);
        // FIXME: the problem is trying to access to this structure
        myPacketHeader *inner = &(buffer[ord_no].packetHeader);
        inner->ord_no = ord_no;
        buffer[ord_no].payload = calloc(1, MAX_PAYLOAD_SIZE);
        // copying the data in the actual payload
        memcpy(buffer[ord_no].payload, data, MAX_PAYLOAD_SIZE);
        // TODO: are we sure we want to create the same packet every time?
        
        // Check if going forward of bytes or what
        data += MAX_PAYLOAD_SIZE;
    }
    return buffer;
}

// with some ipv6 packets try to reconstruct everything
void *reconstruct(ipv6Packet *data, int len) {
    // if checksum is correct than...
    int i;
    void *rebuild = calloc(len, MAX_PAYLOAD_SIZE);
    printf ("allocated %d memory for this\n", len * MAX_PAYLOAD_SIZE);
    void *idx = rebuild;
    for (i = 0; i < len; i++) {
        // checksum before
        memcpy(idx, data[i].payload, MAX_PAYLOAD_SIZE);
        idx += MAX_PAYLOAD_SIZE;
    }
    return rebuild;
}

// Function for checksum calculation. From the RFC,
// the checksum algorithm is:
//  "The checksum field is the 16 bit one's complement of the one's
//  complement sum of all 16 bit words in the header.  For purposes of
//  computing the checksum, the value of the checksum field is zero."
unsigned short csum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for(sum=0; nwords>0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
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
