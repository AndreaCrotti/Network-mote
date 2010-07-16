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
    int fd = tun_open("tap0", IFF_TAP);
    if (fd < 0) {
        PERROR("can't open interface");
    }
    char tun_name[IFNAMSIZ];

    // The IP address
    char *ip_address_str = "10.0.0.1";

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    int tun_fd = tun_open(tun_name, IFF_TAP);
    if (tun_fd < 1) {
        printf("Could not create tunnel device. Fatal.\n");
        return 1;
    } else {
        printf("created tunnel device: %s\n", tun_name);
    }

    // Setup the tunnel (Beneath other things, this sets the ip address)
    if (tun_setup(tun_name, ip_address_str) < 0) {
        printf("configuring the tun failed; aborting\n");
        return 1;
    }

    char *buf = "very long string\n";
    // otherwise try to use tun_read to get some data
    int ret = tun_write(fd, buf, strlen(buf));

    return 0;
}
