// -*- compile-command: "gcc -Wall chunker.c -o chunker" -*-
// TODO: add one End Of Packet packet which means end of transmission for packet with seq_no
// TODO: check integer division
// TODO: send the raw packets generated via the tun device
// TODO: use perror to print out errors when possible
// TODO: set first the packet to all 0 and only set the needed fields

#include <stdlib.h>
#include <stdio.h>
#include <netinet/ip6.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>

// measures in bytes
#define SIZE_IPV6_HEADER 40
#define MAX_CARRIED 127
#define TOT_PACKET_SIZE(payload_len) (sizeof(ip6_hdr) + sizeof(myPacketHeader) + payload_len)

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

// TODO: is this the best way to solve this problem?
int MAX_PAYLOAD_SIZE = MAX_CARRIED - sizeof(myPacketHeader) - sizeof(struct ip6_hdr);

ipv6Packet *genIpv6Packets(void *, int, int);
void *reconstruct(ipv6Packet *, int);
unsigned short csum(unsigned short *, int);
void dataToLocalhost(void *, int, int);
ip6_hdr genIpv6Header(size_t);
sockaddr_in6 *localhostDest(void);
void testWithMemset(void);
void sendToLocalhost(void *, size_t);
void sendDataTo(void *, struct sockaddr *, size_t, int);

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
    printf("total length of packet %ld\n", TOT_PACKET_SIZE(MAX_PAYLOAD_SIZE));
    /* dataToLocalhost(arr, num_chunks, 0); */
    testWithMemset();
    return 0;
}

void testWithMemset(void) {
    unsigned char *buff = calloc(MAX_CARRIED, sizeof(unsigned char));
    ip6_hdr header = genIpv6Header(100);
    memcpy(buff, &header, sizeof(header));
    int *data = calloc(10, sizeof(int));
    memset(data, 0xff, sizeof(int) * 10);
    memcpy(buff + sizeof(header), data, sizeof(int) * 10);
    sendToLocalhost(buff, TOT_PACKET_SIZE(MAX_PAYLOAD_SIZE));
}

// how do I free the memory when not returning pointers?
// FIXME: maybe we have to use htons whenever we add data to the network
ip6_hdr genIpv6Header(size_t payload_clen) {
    ip6_hdr header;
    printf("payload len = %ld\n", payload_len);
    header.ip6_src = in6addr_loopback;
    header.ip6_dst = in6addr_loopback;
    header.ip6_ctlun.ip6_un1.ip6_un1_plen = payload_len;
    header.ip6_ctlun.ip6_un2_vfc = 6;
    /* header->ip6_src = 0; */
    return header;
}

sockaddr_in6 *localhostDest(void) {
    sockaddr_in6 *dest = malloc(sizeof(sockaddr_in6));
    dest->sin6_family = AF_INET6;
    // manual way to set the loopback interface
    dest->sin6_addr = in6addr_loopback;

    dest->sin6_addr.s6_addr16[7] = htons(1);
    dest->sin6_flowinfo = 0;
    dest->sin6_scope_id = 0;
    /* dest.sin6_port = htons(9999); */
    dest->sin6_port = 0;
    return dest;
}

void sendDataTo(void *buffer, struct sockaddr *dest, size_t size, int raw_sock) {
    // actually sending away my data with given length
    printf("sizeof(dest) = %d\n", sizeof(*dest));
    // FIXME: why the hell we get this error, if getting dest from here it doesn't work
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
        assert(result == size);
    }
}


// using struct and not pointers can be more heavy but no free necessary
void sendToLocalhost(void *buffer, size_t size) {
    sockaddr_in6 *dest = localhostDest();
    // send to localhost simply using a raw socket
    int raw_sock;

    // creating a raw socket for ipv6
    if((raw_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket() error");
    }
    // This could be dangerous since we use outside a pointer to a local variable
    sendDataTo(buffer, (struct sockaddr *) dest, size, raw_sock);
    /* free(dest); */
}

// create some data and send it 
void dataToLocalhost(void *data, int num_chunks, int seq_no) {
    ipv6Packet *buffer = genIpv6Packets(data, num_chunks, seq_no);

    // send to localhost simply using a raw socket
    int raw_sock;
    
    if((raw_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket() error");
    }

    // now set up the correct fields to it

    int i;
    for (i = 0; i < num_chunks; i++) {
        sendToLocalhost(&buffer, TOT_PACKET_SIZE(MAX_PAYLOAD_SIZE));
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
    // the header is actually correct
    assert(original.ip6_hdr.ip6_ctlun.ip6_un1.ip6_un1_plen == (sizeof(myPacketHeader) + MAX_PAYLOAD_SIZE));
    // set up some common fields
    myPacketHeader pkt_header = original.packetHeader;
    pkt_header.seq_no = seq_no;
    // setup here the ipv6 fields

    // only the payload and the ord_no are changing
    for (ord_no = 0; ord_no < num_chunks; ord_no++) {
        // simply copying by value the original packet
        buffer[ord_no] = original;
        assert(buffer[ord_no].ip6_hdr.ip6_ctlun.ip6_un1.ip6_un1_plen == (sizeof(myPacketHeader) + MAX_PAYLOAD_SIZE));
        // should work anyway?
        printf("copied the memory correctly, ord_no = %d\n", ord_no);
        buffer[ord_no].packetHeader.ord_no = ord_no;
        buffer[ord_no].payload = calloc(1, MAX_PAYLOAD_SIZE);
        // copying the data in the actual payload
        memcpy(buffer[ord_no].payload, data, MAX_PAYLOAD_SIZE);
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

