// -*- compile-command: "gcc -Wall -DSTANDALONE chunker.c -o chunker" -*-
// TODO: add one End Of Packet packet which means end of transmission for packet with seq_no
// TODO: check integer division
// TODO: send the raw packets generated via the tun device
// TODO: use perror to print out errors when possible
// TODO: set first the packet to all 0 and only set the needed fields

#include "chunker.h"
#include "structs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>

#include "motecomm.h"


// FIXME: maybe we have to use htons whenever we add data to the network
void genIpv6Header(ip6_hdr *const header, size_t payload_len) {
    // FIXME: src and dest are surely not loopback address
    header->ip6_src = in6addr_loopback;
    header->ip6_dst = in6addr_loopback;
    // 16 bit file
    header->plen = htons(payload_len);
}

/** 
 * Computes the needed number of chunks given a payload size.
 * 
 * @param data_size The payload size.
 * 
 * @return The number of needed chunks.
 */
unsigned neededChunks(int data_size){
    return ((data_size + MAX_CARRIED-1)/MAX_CARRIED);
}


// FIXME: obsolete comment below
/** 
 * Splits data into chunks and stores them in the packet.
 * 
 * @param payload Contains datastream and total length
 * @param packet Pointer to the packet to write the result to
 * @param seq_no Sequential number to use
 *
 * @returns how may packets remain - You should loop until its 0.
 */
int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number) {
    assert(packet);
    assert(payload);
    assert(payload->len > 0);
    
    // static because we want to keep its value through different calls
    static struct myPacketHeader pkt = {
        .seq_no = 0xFF,
        .ord_no = 0xFF
    };

    // initialized if it's a new one
    if (pkt.seq_no != seq_no) {
        printf("creating a new packet %d\n", seq_no);
        pkt.seq_no = seq_no;
        pkt.ord_no = 0;
        pkt.parts = chunk_number;
    }

    packet->header.packetHeader = pkt;
    pkt.ord_no++;
    *sendsize = (payload->len < MAX_CARRIED) ? (payload->len) : MAX_CARRIED;
    // setup the ipv6 we need
    genIpv6Header(&(packet->header.ip6_hdr), sizeof(myPacketHeader) + *sendsize);
    memcpy(packet->payload, payload->stream, *sendsize);
    payload->len -= *sendsize;
    payload->stream += *sendsize;
    // no cleaner way to set this??
    *sendsize += sizeof(struct ipv6PacketHeader);

    return (payload->len+MAX_CARRIED-1)/MAX_CARRIED;
}

/* void genIpv6Packets2(payload_t *const payload, ipv6Packet *const result, int const seq_no, const int parts) { */
/*     assert(result); */
/*     // why not just payload->len?? */
/*     unsigned sendsize = (payload->len < MAX_CARRIED) ? (payload->len) : MAX_CARRIED; */

/*     for (int i = 0; i < parts; i++) { */
/*         myPacketHeader pkt = { */
/*             .seq_no = seq_no, */
/*             .ord_no = i, */
/*             .parts = parts */
/*         }; */
        
/*         result[i].header.packetHeader = pkt; */
/*         genIpv6Header(&(result[i].header.ip6_hdr), sizeof(myPacketHeader) + sendsize); */
/*         memcpy(result[i].payload, payload.stream, *sendsize); */
/*     } */
/* } */


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

#if STANDALONE
int main(int argc, char **argv) {
    ipv6Packet v6;
    printf("v6 = %ld\n", sizeof(v6));
    int size = 1000;
    // now we try to send the packet and see if it's sniffable
    int *arr = calloc(sizeof(int), size);
    for (int i = 0; i < size; i++) {
        arr[i] = i;
    }
    int num_chunks = neededChunks(size);
    // testWithMemset(); 
    return 0;
}

#endif
