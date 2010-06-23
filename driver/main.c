
// Includes
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>

// Standart libraries
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Tunnel device
#include "tun_dev.h"

int main(int args, char** arg) {
    char dev[IFNAMSIZ];
    char input[20];

    // a new device should be opened!
    dev[0] = 0;
    
    int fd = tun_open(dev);
    
    //fgets(input, 20, stdin);

    uint8_t buf[sizeof(struct split_ip_msg) + INET_MTU];
    struct split_ip_msg *msg = (struct split_ip_msg *)buf;
    int len;

    while(1){
        len = tun_read(fd, (void *)(&msg->pi), INET_MTU + sizeof(struct ip6_hdr));
        if(len != -5)
            printf("%d", len);
    }

    tun_close(fd, dev);

    return 0;
}
