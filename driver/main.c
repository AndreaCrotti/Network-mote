#include <sys/socket.h>
#include <tun.h>
#include <linux/if.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int args, char** arg) {
    char dev[IFNAMSIZ];
    memset(dev, 0, IFNAMSIZ);
    *dev = 42;
    int fd = tun_open(dev);
    if (fd < 1) {
        printf("hat nich geklappt! :(\n");
    } else {
        char cpy[IFNAMSIZ + 2];
        memset(cpy, 0, IFNAMSIZ+2);
        strncpy(cpy, dev, IFNAMSIZ);
        printf("whooohooo: %s\n", cpy);
    }
    return 0;
}
