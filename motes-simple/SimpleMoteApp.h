/**
 * @file SimpleMoteApp.h
 * @author Marius Grysla
 * @date Fr 23. Juli 10:00:23 CEST 2010
 *
 * @brief This file carries configuration variables and structs needed in the
 *        SimpleMoteApp program.
 *
 **/

#ifndef SIMPLEMOTEAPP_H
#define SIMPLEMOTEAPP_H

#include "AM.h"

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

typedef nx_uint8_t nx_seq_no_t;
typedef nx_uint8_t nx_boolean;

typedef uint8_t seq_no_t;
typedef uint8_t boolean;

// WARNING must be the same as the one in structs.h
typedef nx_struct myPacketHeader {
    nx_am_addr_t sender;
    nx_am_addr_t destination;
    nx_seq_no_t seq_no;
    nx_uint8_t ord_no;
    // tells if the payload is compressed or not
    nx_boolean is_compressed;
    // how many chunks in total
    nx_uint8_t parts;
} myPacketHeader;

#endif
