#ifndef _CLIENT_H
#define _CLIENT_H

/*************************/
/* Function declarations */
/*************************/
/* void print_ip_packet(struct split_ip_msg *msg); */

// The IP address for our tunnel device
/* struct in6_addr __my_address = {{{0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, */
/*                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65}}}; */

// XXX seriously, this is not how you do things in C
enum {
    FALSE = 0,
    TRUE = 1,
    BUFSIZE = 256,
    MTU = 256,
};

#endif
