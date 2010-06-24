
// Includes
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Functions for using a tunnel device
#include "tun_dev.h"
// For adding an entry to the routing tables
#include "routing.h"

extern struct in6_addr __my_address;

// The IP address for our tunnel device
struct in6_addr __my_address       = {{{0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65}}};

int main(int args, char** arg) {
    char dev[IFNAMSIZ];
    char input[20];

    // a new device should be opened!
    dev[0] = 0;    
    // create the tunnel
    int fd = tun_open(dev);
    if (fd < 1) {
      printf("Could not create tunnel device. Fatal.\n");
      return 1;
    } else {
      printf("created tun device: %s\n", dev);
    }

    // Setup the tunnel
    //NOTE: Does this set the IP-address?
    if (tun_setup(dev, &__my_address) < 0) {
      printf("configuring the tun failed; aborting\n");
      return 1;
    }

    // Testing...
    printf("IPV6_VERSION is: %d", IPV6_VERSION);
    fflush(stdout);

    uint8_t buf[sizeof(struct split_ip_msg) + INET_MTU];
    struct split_ip_msg *msg = (struct split_ip_msg *)buf;
    int len;

    while (1) {
        len = tun_read(fd, (void *)(&msg->pi), INET_MTU + sizeof(struct ip6_hdr));
        //if(len != -5)
            //printf("%d", len);

        if (len > 0) {
            printf("tun_read: read 0x%x bytes\n", len);

            if ((msg->hdr.vlfc[0] >> 4) != IPV6_VERSION) {
                printf("tun_read: discarding non-ip packet\n");
            } else if (ntohs(msg->hdr.plen) > INET_MTU - sizeof(struct ip6_hdr)) {
                printf("tun_input: dropping packet due to length: 0x%x\n", ntohs(msg->hdr.plen));
            } else if (msg->hdr.nxt_hdr == 0) {
                printf("tun_input: dropping packet with IPv6 options\n");
            }
        }
        fflush(stdout);
        usleep(100);
    }

    tun_close(fd, dev);

    return 0;
}
