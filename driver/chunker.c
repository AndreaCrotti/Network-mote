// -*- compile-command: "gcc -Wall -DSTANDALONE chunker.c -o chunker" -*-
// TODO: add one End Of Packet packet which means end of transmission for packet with seq_no
// TODO: check integer division
// TODO: send the raw packets generated via the tun device
// TODO: use perror to print out errors when possible
// TODO: set first the packet to all 0 and only set the needed fields

#include "chunker.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>

#include "motecomm.h"

#define MAX_IPVS_SIZE (MAX_CARRIED - sizeof(struct ipv6PacketHeader))

/*void testWithMemset(void) {
    unsigned char *buff = calloc(MAX_CARRIED, sizeof(unsigned char));
    ip6_hdr header;
    genIpv6Header(&header,100);
    memcpy(buff, &header, sizeof(ip6_hdr));
    sendToLocalhost(buff, TOT_PACKET_SIZE(MAX_IPVS_SIZE));
}*/


// FIXME: maybe we have to use htons whenever we add data to the network
void genIpv6Header(ip6_hdr *const header, size_t payload_len) {
    header->ip6_src = in6addr_loopback;
    header->ip6_dst = in6addr_loopback;
    // 16 bit file
    header->ip6_ctlun.ip6_un1.ip6_un1_plen = htons(payload_len);
    printf("payload len = %x, after htons %x\n", payload_len, htons(payload_len));
    header->ip6_ctlun.ip6_un2_vfc = 6;
    /* header->ip6_src = 0; */
}

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


// using struct and not pointers can be more heavy but no free necessary
/*void sendToLocalhost(void *buffer, size_t size) {
    sockaddr_in6 *dest = localhostDest();
    // send to localhost simply using a raw socket
    int raw_sock;

    // creating a raw socket for ipv6
    if((raw_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket() error");
    }
    // This could be dangerous since we use outside a pointer to a local variable
    sendDataTo(buffer, (struct sockaddr *) dest, size, raw_sock);
    // free(dest); 
}*/

// create some data and send it 
/*void dataToLocalhost(void *data, int num_chunks, int seq_no) {
    ipv6Packet *buffer = genIpv6Packet(data, num_chunks, seq_no);

    // send to localhost simply using a raw socket
    int raw_sock;
    
    if((raw_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket() error");
    }

    // now set up the correct fields to it

    int i;
    for (i = 0; i < num_chunks; i++) {
        printf("sending packet number %d\n", i);
        sendToLocalhost(&buffer, TOT_PACKET_SIZE(MAX_IPVS_SIZE));
        buffer++;
    }
}*/

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
int genIpv6Packet(payload_t* const payload, ipv6Packet* const packet, int const seq_no) {
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
    packet->plsize = packet->sendsize = (payload->len < MAX_CARRIED)?payload->len:MAX_CARRIED;
    packet->sendsize += sizeof(struct ipv6PacketHeader);
    memcpy(packet->payload,payload->stream,packet->plsize);
    payload->len -= packet->plsize;
    payload->stream += packet->plsize;
    return (payload->len+MAX_CARRIED-1)/MAX_CARRIED -1;
} 

/** 
 * Generate an array of ipv6Packet ready to send over the network
 * 
 * @param data data to split and encapsulate in chunks
 * @param data_size size of the payload to send.
 * 
 * @return 
 *
ipv6Packet *genIpv6Packets(void *data, int data_size, int seq_no, unsigned* count) {
    unsigned num_chunks = ((size + MAX_CARRIED-1/MAX_CARRIED));
    *count = num_chunks;

    int ord_no;
    // check this "integer" division
    // generating an array of ipv6Packet of the correct length
    // TODO: check if this size is actually correct
    ipv6Packet *buffer = calloc(num_chunks, sizeof(ipv6Packet));
    // copy this every time we need a new one
    ipv6Packet original;
    ip6_hdr *header = genIpv6Header(sizeof(myPacketHeader) + MAX_CARRIED);
    memcpy(&(original.ip6_hdr), header, sizeof(ip6_hdr));
    // check that the payload length is set correctly
    printf("%d instead of %d\n", original.ip6_hdr.ip6_ctlun.ip6_un1.ip6_un1_plen, (sizeof(myPacketHeader) + MAX_CARRIED));
    // assert(original.ip6_hdr.ip6_ctlun.ip6_un1.ip6_un1_plen == (sizeof(myPacketHeader) + MAX_IPVS_SIZE)); 
    // set up some common fields
    myPacketHeader pkt_header = original.packetHeader;
    pkt_header.seq_no = seq_no;
    // setup here the ipv6 fields

                            
    for (ord_no = 0; ord_no < num_chunks; ord_no++) {
        // simply copying by value the original packet
        buffer[ord_no] = original;
        assert(buffer[ord_no].ip6_hdr.ip6_ctlun.ip6_un1.ip6_un1_plen == (sizeof(myPacketHeader) + MAX_CARRIED));
        // should work anyway?
        buffer[ord_no].packetHeader.ord_no = ord_no;
        int copy_len;
        if(ord_no == (num_chunks - 1) && ord_no * MAX_CARRIED + MAX_CARRIED > data_size){
            int data_left = data_size - ord_no * MAX_CARRIED;
            copy_len = data_left;
        }else{  
            copy_len = MAX_CARRIED;
        }
        buffer[ord_no].payload = malloc(copy_len);
        memcpy(buffer[ord_no].payload, data, copy_len);
        buffer[ord_no].plsize = copy_len;
        data += copy_len;
    }
    return buffer;
}*/

// with some ipv6 packets try to reconstruct everything
void *reconstruct(ipv6Packet *data, int len) {
    // if checksum is correct than...
    int i;
    void *rebuild = calloc(len, MAX_IPVS_SIZE);
    printf ("allocated %d memory for this\n", len * MAX_IPVS_SIZE);
    void *idx = rebuild;
    for (i = 0; i < len; i++) {
        // checksum before
        memcpy(idx, data[i].payload, MAX_IPVS_SIZE);
        idx += MAX_IPVS_SIZE;
    }
    return rebuild;
}

// Function for checksum calculation. From the RFC,
// the checksum algorithm is:
//  "The checksum field is the 16 bit one's complement of the one's
//  complement sum of all 16 bit words in the header.  For purposes of
//  computing the checksum, the value of the checksum field is zero."
/*unsigned short csum(unsigned short *buf, int nwords) {
    unsigned long sum;
    for(sum=0; nwords>0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum &0xffff);
    sum += (sum >> 16);
    return (unsigned short)(~sum);
}*/// XXX: see split.c <-> same symbol

#if STANDALONE
/*
int main(int argc, char **argv) {
    ipv6Packet v6;
    printf("v6 = %ld\n", sizeof(v6));
    // now we try to send the packet and see if it's sniffable
    int i;

    int *arr = calloc(sizeof(int), 1000);
    for (i = 0; i < 1000; i++) {
        arr[i] = i;
    }
    int num_chunks = (int)(1000 / MAX_IPVS_SIZE) + 1;
    printf("total length of packet %ld\n", TOT_PACKET_SIZE(MAX_IPVS_SIZE));
    dataToLocalhost(arr, num_chunks, 0);
    / testWithMemset(); 
    return 0;
}
*/

#endif
