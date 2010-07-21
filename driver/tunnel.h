
#ifndef __TUNNEL_H
#define __TUNNEL_H

#include <arpa/inet.h>

#define MAX_QUEUED 10

// structure of a tun device
typedef struct tundev {
    char *ifname;
    int fd;
    int client; // client we're serving
} tundev;

/// queue of messages to write on it
/// read of course doesn't have this problem
struct {
    char *messages[MAX_QUEUED];
} queue;

/*************************/
/* Function declarations */
/*************************/
int tun_open(int client_no, char *dev, int flags);
int tun_read(int fd, char *buf, int n);
int tun_write(int fd, char *buf, int n);

#endif
