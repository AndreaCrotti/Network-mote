// -*- compile-command: "gcc -Wall -o try try.c tunnel.c" -*-
// Includes
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <termios.h>
#include <arpa/inet.h>

// Standard libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tunnel.h"

#define PERROR(x) { perror(x); exit(1); }

int main() {
    // and then try to write and read something
    /* int fd = tun_open("tap0", IFF_TAP); */
    /* if (fd < 0) { */
    /*     PERROR("can't open interface"); */
    /* } */
    /* char tun_name[IFNAMSIZ]; */

    /* // The IP address */
    /* char *ip_address_str = "10.0.0.1"; */

    /* // a new device should be opened! */
    /* tun_name[0] = 0;     */
    /* // create the tap-device */
    /* int tun_fd = tun_open(tun_name, IFF_TAP); */
    /* if (tun_fd < 1) { */
    /*     printf("Could not create tunnel device. Fatal.\n"); */
    /*     return 1; */
    /* } else { */
    /*     printf("created tunnel device: %s\n", tun_name); */
    /* } */

    /* // Setup the tunnel (Beneath other things, this sets the ip address) */
    /* if (tun_setup(tun_name, ip_address_str) < 0) { */
    /*     printf("configuring the tun failed; aborting\n"); */
    /*     return 1; */
    /* } */
    int fd = open("/dev/net/tun", O_WRONLY);
    
    char buf[100] = "very long string\n";
    // otherwise try to use tun_read to get some data
    int ret = write(fd, buf, 100);
    if (ret < 0) {
        PERROR("could not write on the tap device");
    }
    return 0;
}

/* int my_tun_open(char *dev, int flags){ */
    
/*     struct ifreq ifr; */
/*     int fd, err; */
/*     char *clonedev = "/dev/net/tun"; */
    
/*     // Open the clone device */
/*     if( (fd = open(clonedev , O_RDWR)) < 0 ) { */
/*         perror("Opening /dev/net/tun"); */
/*         return fd; */
/*     } */
    
/*     // prepare ifr */
/*     memset(&ifr, 0, sizeof(ifr)); */
/*     // Set the flags */
/*     ifr.ifr_flags = flags; */

/*     // If a device name was specified it is put to ifr */
/*     if(*dev){ */
/*         strncpy(ifr.ifr_name, dev, IFNAMSIZ); */
/*     } */

/*     // Try to create the device */
/*     if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){ */
/*         close(fd); */
/*         perror("Creating the device"); */
/*         return err; */
/*     } */

/*     // Write the name of the new interface to device */
/*     strcpy(dev, ifr.ifr_name); */

/*     // allocate 10 Bytes for the interface name */
/*     ifname = malloc(10); */

/*     return fd; */
/* } */
