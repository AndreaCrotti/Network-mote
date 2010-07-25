/**
 * Some function utilities to debug structs
 * 
 */
#include <stdio.h>
#include <stdlib.h>

#include "../shared/structs.h"

void printPayload(payload_t t) {
    printf("payload with length = %d\n", t.len);
}

void printPacketHeader(myPacketHeader *pkt) {
    printf("(seq = %d, ord = %d, parts = %d)\n", pkt->seq_no, pkt->ord_no, pkt->parts);
}

void printIpv6Header(ip6_hdr header) {
    printf("len = %d, src...\n", header.plen);
}

void printIpv6Packet(ipv6Packet *pkt) {
    printPacketHeader(&((pkt->header).packetHeader));
    printIpv6Header((pkt->header).ip6_hdr);
}
