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

#define MAX_IPVS_SIZE (MAX_CARRIED - sizeof(struct ipv6PacketHeader))

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
unsigned needed_chunks(int data_size){
    return ((data_size + MAX_CARRIED-1)/MAX_CARRIED);
}

/** 
 * Splits data into chunks and stores them in the packet.
 * 
 * @param payload Contains datastream and total length
 * @param packet Pointer to the packet to write the result to
 * @param seq_no Sequential number to use
 *
 * @returns how may packets remain - You should loop until its 0.
 */
int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, unsigned* sendsize, int const seq_no) {
    assert(packet);
    assert(payload);
    assert(payload->len > 0);
    static struct myPacketHeader pkt = {.seq_no = 0xFF, .ord_no = 0xFF, .checksum = 0};

    if (pkt.seq_no != seq_no) {
      pkt.seq_no = seq_no;
      pkt.ord_no = 0;
    }

    genIpv6Header(&(packet->header.ip6_hdr),sizeof(myPacketHeader) + MAX_CARRIED /* FIXME: is this actually correct?*/);
    packet->header.packetHeader = pkt;
    pkt.ord_no++;
    *sendsize = (payload->len < MAX_CARRIED)?payload->len:MAX_CARRIED;
    memcpy(packet->payload,payload->stream,*sendsize);
    payload->len -= *sendsize;
    payload->stream += *sendsize;
    *sendsize += sizeof(struct ipv6PacketHeader);
    return (payload->len+MAX_CARRIED-1)/MAX_CARRIED;
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

#if STANDALONE
int main(int argc, char **argv) {
    ipv6Packet v6;
    printf("v6 = %ld\n", sizeof(v6));
    // now we try to send the packet and see if it's sniffable
    int *arr = calloc(sizeof(int), 1000);
    for (int i = 0; i < 1000; i++) {
        arr[i] = i;
    }
    int num_chunks = (int)(1000 / MAX_IPVS_SIZE) + 1;
    printf("total length of packet %ld\n", TOT_PACKET_SIZE(MAX_IPVS_SIZE));
    dataToLocalhost(arr, num_chunks, 0);
    / testWithMemset(); 
    return 0;
}

#endif

/*sockaddr_in6 *localhostDest(void) {
    sockaddr_in6 *dest = malloc(sizeof(sockaddr_in6));
    dest->sin6_family = AF_INET6;
    // manual way to set the loopback interface
    dest->sin6_addr = in6addr_loopback;

    dest->sin6_addr.s6_addr16[7] = htons(1);
    dest->sin6_flowinfo = 0;
    dest->sin6_scope_id = 0;
    // dest.sin6_port = htons(9999); 
    dest->sin6_port = 0;
    return dest;
}*/

/*void sendDataTo(void *buffer, struct sockaddr *dest, size_t size, int raw_sock) {
    // TODO: the sizes are different, how does it manage automatically
    printf("norm %u, 6version %u,\n", sizeof(struct sockaddr), sizeof(sockaddr_in6));
    int result = sendto(raw_sock,
                        &buffer,
                        size,
                        0,
                        dest,
                        // TODO: this is not generic enough
                        sizeof(sockaddr_in6));
    if (result < 0) {
        perror("error in sending");
    } else {
        // checking if sending all the data needed
        assert((unsigned)result == size);
    }
}*/
