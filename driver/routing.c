#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <lib6lowpan/6lowpan.h>
#include <lib6lowpan/lib6lowpan.h>
#include "routing.h"
#include "config.h"

// Some definitions
static ieee154_saddr_t my_short_addr;
static uint16_t current_seqno;
extern struct in6_addr  __my_address;

// Global variables
char proxy_dev[IFNAMSIZ], tun_dev[IFNAMSIZ];
int mcast_sock;

// initialize the routing structure to do it directly from here
int routing_init(struct config *c, char *tun_name) {
    FILE *fd;
    char buf[256];
    my_short_addr = ntohs(__my_address.s6_addr16[7]);
    strncpy(proxy_dev, c->proxy_dev, IFNAMSIZ);
    strncpy(tun_dev, tun_name, IFNAMSIZ);

    // set up the network state data structures
    //nw_init();

    // start a netlink session to the kernel
    //nl_init();

    //mcast_sock = mcast_start(proxy_dev);;

    // 
    if ((fd = fopen("/proc/sys/net/ipv6/conf/all/forwarding", "w")) == NULL) {
        //log_fatal_perror("enable forwarding");
        return -1;
    }
    fprintf(fd, "1");
    fclose(fd);

    snprintf(buf, sizeof(buf), "/proc/sys/net/ipv6/conf/%s/proxy_ndp", proxy_dev);
    if ((fd = fopen(buf, "w")) == NULL) {
        //warn("unable to enable IPv6 ND proxy on %s\n", proxy_dev);
    } else {
        fprintf(fd, "1");
        fclose(fd);
    }

    if ((fd = fopen("/var/run/ip-driver.seq", "r")) != NULL) {
        if (fscanf(fd, "%hi\n", &current_seqno) != 1) {
            current_seqno = 0;
        }
        fclose(fd);
    }

    return (mcast_sock >= 0) ? 0 : -1;
}
