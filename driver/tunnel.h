
#ifndef __TUNNEL_H
#define __TUNNEL_H

#include <arpa/inet.h>

#define MAX_QUEUED 10

// implementation of a FIFO queue as a circular array
typedef struct write_queue {
    char *messages[MAX_QUEUED];
    int head;
    int bottom;
} write_queue;

// structure of a tun device
typedef struct tundev {
    char *ifname;
    int fd;
    int client; // client we're serving
    write_queue queue;
} tundev;


/*************************/
/* Function declarations */
/*************************/
int tun_open(int client_no, char *dev, int flags);
int tun_read(int fd, char *buf, int n);
int tun_write(int fd, char *buf, int n);
void addToWriteQueue(int client_no, char *buf, int len);

#endif
