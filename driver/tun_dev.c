#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_ether.h>

#include <netinet/in.h>
/* #include <lib6lowpan/lib6lowpan.h> */
/* #include <driver/logging.h> */

#include "tun_dev.h"


/*
 *    This is in linux/include/net/ipv6.h.
 *    Thanks, net-tools!
 */
struct in6_ifreq {
    struct in6_addr ifr6_addr;
    __u32 ifr6_prefixlen;
    unsigned int ifr6_ifindex;
};


void log_fatal_perror(const char* string){
    printf("%s", string);
}

int tun_open(char *dev)
{
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR | O_NONBLOCK)) < 0)
	return -1;

    memset(&ifr, 0, sizeof(ifr));
    /* By default packets are tagged as IPv4. To tag them as IPv6,
     * they need to be prefixed by struct tun_pi.
     */
    //ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    ifr.ifr_flags = IFF_TUN;
    if (*dev)
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if (ioctl(fd, TUNSETIFF, (void *) &ifr) < 0)
	goto failed;

    strcpy(dev, ifr.ifr_name);
    return fd;

  failed:
    log_fatal_perror("tun_open");
    close(fd);
    return -1;
}

int tun_setup(char *dev, struct in6_addr *addr) {
    struct in6_ifreq ifr6;
    struct ifreq ifr;
    int fd;

    if ((fd = socket(PF_INET6, SOCK_DGRAM, 0)) < 0)
        return -1;

    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    /* set the interface up */
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        log_fatal_perror("SIOCGIFFLAGS");
        return -1;
    }
    ifr.ifr_flags |= IFF_UP;
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        log_fatal_perror("SIOCSIFFLAGS");
        return -1;
    }

    /* MTU */
    ifr.ifr_mtu = 1280;
    if (ioctl(fd, SIOCSIFMTU, &ifr) < 0) {
        log_fatal_perror("SIOCSIFMTU");
        return -1;
    }

    /* Global address */
    memset(&ifr6, 0, sizeof(struct in6_ifreq));
    memcpy(&ifr6.ifr6_addr, addr, 16);
    if (ioctl(fd, SIOGIFINDEX, &ifr) < 0) {
        log_fatal_perror("SIOGIFINDEX");
        return -1;
    }

    ifr6.ifr6_ifindex = ifr.ifr_ifindex;
    ifr6.ifr6_prefixlen = 128;

    // Commented out, since this seems to give an error if called twice 
    /* if (ioctl(fd, SIOCSIFADDR, &ifr6) < 0) { */
    /*     log_fatal_perror("SIOCSIFADDR (global)"); */
    /*     return -1; */
    /* } */

    memset(&ifr6.ifr6_addr.s6_addr[0], 0, 16);
    ifr6.ifr6_addr.s6_addr16[0] = htons(0xfe80);
    ifr6.ifr6_addr.s6_addr16[7] = addr->s6_addr16[7];
  
    if (ioctl(fd, SIOCSIFADDR, &ifr6) == -1) {
        log_fatal_perror("SIOCSIFADDR (local)");
        return -1;
    }

    close(fd);

    return 0;
}

int tun_close(int fd, char *dev)
{
    return close(fd);
}

/* Read/write frames from TUN device */
int tun_write(int fd, struct split_ip_msg *msg)
{
    uint8_t buf[INET_MTU + sizeof(struct tun_pi)], *packet;
    struct tun_pi *pi = (struct tun_pi *)buf;
    struct generic_header *cur;
    packet = (uint8_t *)(pi + 1);


    if (ntohs(msg->hdr.plen) + sizeof(struct ip6_hdr) >= INET_MTU)
        return 1;

    pi->flags = 0;
    pi->proto = htons(ETH_P_IPV6);

    memcpy(packet, &msg->hdr, sizeof(struct ip6_hdr));
    packet += sizeof(struct ip6_hdr);

    cur = msg->headers;
    while (cur != NULL) {
        memcpy(packet, cur->hdr.data, cur->len);
        packet += cur->len;
        cur = cur->next;
    }

    memcpy(packet, msg->data, msg->data_len);

    return write(fd, buf, sizeof(struct tun_pi) + sizeof(struct ip6_hdr) + ntohs(msg->hdr.plen));
}

// we pass the memory already allocated?
int tun_read(int fd, char *buf, int len)
{
    int out;
    out = read(fd, buf, sizeof(struct tun_pi) + len);
    
    return out - sizeof(struct tun_pi);
}
