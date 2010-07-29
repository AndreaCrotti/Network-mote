/**
 * Split the incoming data in chunks maximizing the dimension used
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>

#include "chunker.h"
#include "structs.h"
#include "motecomm.h"

/** 
 * Generate a correct ipv6 header, this is the BLIP ipv6 structure!
 * 
 * @param header header to overwrite
 * @param payload_len length of the payload
 */
void genIpv6Header(ip6_hdr *const header, size_t payload_len) {
    header->ip6_src = in6addr_loopback;
    header->ip6_dst = in6addr_loopback;
    // 16 bit file, so we need to convert to network format
    header->plen = htons(payload_len);
}

/** 
 * Computes the needed number of chunks given a payload size.
 * 
 * @param data_size The payload size.
 * 
 * @return The number of needed chunks.
 */
unsigned neededChunks(int data_size) {
    return ((data_size + MAX_CARRIED-1)/MAX_CARRIED);
}

// TODO: comment in chunker.h
int genPacket(payload_t* const payload, ipv6Packet* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number) {
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
        LOG_DEBUG("creating a new packet %d", seq_no);
        pkt.seq_no = seq_no;
        pkt.ord_no = 0;
        pkt.parts = chunk_number;
    }

    packet->header.packetHeader = pkt;
    pkt.ord_no++;
    *sendsize = (payload->len < MAX_CARRIED) ? (payload->len) : MAX_CARRIED;
    // setup the ipv6 we need
#if !NO_IPV6
    genIpv6Header(&(packet->header.ip6_hdr), sizeof(myPacketHeader) + *sendsize);
#endif
    memcpy(packet->payload, payload->stream, *sendsize);
    payload->len -= *sendsize;
    payload->stream += *sendsize;
    // no cleaner way to set this??
    *sendsize += sizeof(struct ipv6PacketHeader);

    return (payload->len+MAX_CARRIED-1)/MAX_CARRIED;
}

// NOT USED!
// Another implementation of genIpv6Packet which instead takes an array of payloads already allocated
// This could be useful to keep an history if we need to send back some chunks
void genIpv6Packets2(payload_t *const payload, payload_t *const result, int const seq_no, const unsigned parts) {
    assert(result);
    unsigned rem_len = payload->len;
    // FIXME: wrong, this must be set every time and go down!

    unsigned sendsize;
    for (unsigned int i = 0; i < parts; i++) {
        sendsize = (rem_len < MAX_CARRIED) ? rem_len : MAX_CARRIED;

        myPacketHeader pkt = {
            .seq_no = seq_no,
            .ord_no = i,
            .parts = parts
        };

        // memory is already allocate outside
        ipv6Packet *ipv6 = (ipv6Packet *) result[i].stream;
        ipv6->header.packetHeader = pkt;
        result[i].stream = (stream_t *) ipv6;
        result[i].len = sendsize;
#if !NO_IPV6
        genIpv6Header(&(ipv6->header.ip6_hdr), sizeof(myPacketHeader) + sendsize);
#endif
        memcpy(ipv6->payload, payload->stream, sendsize);
    }
}
