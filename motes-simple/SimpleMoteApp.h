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
    MAX_MOTES = 16;
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
    SERIAL_QUEUE_SIZE = 10
};

#endif
