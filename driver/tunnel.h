
#ifndef __TUNNEL_H
#define __TUNNEL_H

#include <arpa/inet.h>

/*************************/
/* Function declarations */
/*************************/
int tun_open(char *dev, int flags);
int tun_read(int fd, char *buf, int n);
int tun_write(int fd, char *buf, int n);

#endif
