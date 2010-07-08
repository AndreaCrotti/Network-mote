#ifndef _CLIENT_H
#define _CLIENT_H

/*************************/
/* Function declarations */
/*************************/
void stderr_msg(serial_source_msg problem);
/* void print_ip_packet(struct split_ip_msg *msg); */
serial_source open_serial_source(const char *device, int baud_rate,
				 int non_blocking,
				 void (*message)(serial_source_msg problem));
static tcflag_t parse_baudrate(int requested);

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

struct serial_source_t {
    int fd;
    int non_blocking;
    void (*message)(serial_source_msg problem);

    /* Receive state */
    struct {
        uint8_t buffer[BUFSIZE];
        int bufpos, bufused;
        uint8_t packet[MTU];
        int in_sync, escaped;
        int count;
        struct packet_list *queue[256]; // indexed by protocol
    } recv;
    struct {
        uint8_t seqno;
        uint8_t *escaped;
        int escapeptr;
        uint16_t crc;
    } send;
};

#endif
