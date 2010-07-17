#ifndef _TUN_DEV_H
#define _TUN_DEV_H

#include <lib6lowpan/ip.h>

// redefinition of all the functions to manipulate TUN device

int tun_open(char *dev);
int tun_close(int fd, char *dev);
int tun_setup(char *dev, struct in6_addr *addr);
int tun_write(int fd, struct split_ip_msg *msg);
int tun_read(int fd, char *buf, int len);

#endif
