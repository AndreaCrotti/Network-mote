/**
 * This file carries configuration variables for the SimpleMoteApp program.
 *
 * @file SimpleMoteApp.h
 * @author Marius Grysla
 * @date Fr 23. Juli 10:00:23 CEST 2010
 **/
#ifndef __SimpleMoteApp_h
#define __SimpleMoteApp_h

/*
 * General definitions.
 */
enum{
    MAX_MOTES = 16
};

/* 
 * AM types for communications.
 * Values are quite arbitrary right now.
 */
enum {
    AM_SIMPLE_RADIO = 15,
    AM_SIMPLE_SERIAL = 0
};

/*
 * Queue sizes
 */
enum {
    RADIO_QUEUE_SIZE = 10,
    SERIAL_QUEUE_SIZE = 10,
    PACKET_QUEUE_SIZE = 16
};

typedef uint8_t seq_no_t;
typedef uint8_t boolean;

// also the internal struct should be packed
typedef struct myPacketHeader {
    //FIXME: am_addr_t should be used, but does not compile for some reason...
    uint16_t sender;
    uint16_t receiver;
    seq_no_t seq_no;
    uint8_t ord_no;
    // tells if the payload is compressed or not
    boolean is_compressed;
    // how many chunks in total
    uint8_t parts;
} __attribute__((__packed__)) myPacketHeader;

#endif
