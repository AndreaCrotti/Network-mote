
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
    dev[0] = 0;

    int fd = tun_open(dev);
    
    fgets(input, 20, stdin);

    tun_close(fd, dev);

    return 0;
}
