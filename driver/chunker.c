/**
 * Split the incoming data in chunks maximizing the dimension used
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <assert.h>
#include <arpa/inet.h>

#include "chunker.h"
#include "structs.h"
#include "motecomm.h"

// Hardcoded sender and destination addresses for the created packets
uint16_t sender_address;
uint16_t destination_address;


/** 
 * Computes the needed number of chunks given a payload size.
 * 
 * @param data_size The payload size.
 * 
 * @return The number of needed chunks.
 */
unsigned needed_chunks(int data_size) {
    return ((data_size + MAX_CARRIED-1)/MAX_CARRIED);
}

int gen_packet(payload_t* const payload, myPacket* const packet, unsigned* sendsize, seq_no_t const seq_no, int const chunk_number) {
    assert(packet);
    assert(payload);
    assert(payload->len > 0);
    
    // static because we want to keep its value through different calls
    static struct myPacketHeader pkt = {
        .seq_no = 0xFF,
        .ord_no = 0xFF
    };

    pkt.sender = htons(sender_address);
    pkt.destination = htons(destination_address);
    
    // initialized if it's a new one
    if (pkt.seq_no != seq_no) {
        LOG_DEBUG("creating a new packet %d", seq_no);
        pkt.seq_no = seq_no;
        pkt.ord_no = 0;
        pkt.parts = chunk_number;
        pkt.is_compressed = payload->is_compressed;
    }
    // this should be always true unless we try to compress some chunks and not compress others
    assert(pkt.is_compressed == payload->is_compressed);

    packet->packetHeader = pkt;
    // increasing ord number on the static struct
    pkt.ord_no++;
    *sendsize = (payload->len < MAX_CARRIED) ? (payload->len) : MAX_CARRIED;
    memcpy(packet->payload, payload->stream, *sendsize);
    payload->len -= *sendsize;
    payload->stream += *sendsize;
    // no cleaner way to set this??
    *sendsize += sizeof(myPacketHeader);

    return (payload->len+MAX_CARRIED-1)/MAX_CARRIED;
}

// NOT USED!
// Another implementation of gen_myPacket which instead takes an array of payloads already allocated
// This could be useful to keep an history if we need to send back some chunks
void gen_myPackets2(payload_t *const payload, payload_t *const result, int const seq_no, const unsigned parts) {
    assert(result);
    unsigned rem_len = payload->len;
    // FIXME: wrong, this must be set every time and go down!

    unsigned sendsize;
    for (unsigned int i = 0; i < parts; i++) {
        sendsize = (rem_len < MAX_CARRIED) ? rem_len : MAX_CARRIED;

        myPacketHeader pkt_header = {
            .seq_no = seq_no,
            .ord_no = i,
            .parts = parts
        };

        // memory is already allocate outside
        myPacket *pkt = (myPacket *) result[i].stream;
        pkt->packetHeader = pkt_header;
        result[i].stream = (stream_t *) pkt;
        result[i].len = sendsize;
        memcpy(pkt->payload, payload->stream, sendsize);
    }
}
