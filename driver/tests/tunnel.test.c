#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>  
#include <net/route.h>
#include <signal.h>
#include <assert.h>

#include "tunnel.h"
#include "structs.h"

// testing the creation and writing/reading from with the tunnel
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    // setup a tun device and then work with it
    int client = 0;
    tunSetup(IFF_TUN);
    char tun_name[IFNAMSIZ];
    tun_name[0] = 0;
    char buff[] = "ciao ciao \0";
    // tunnel for client 0 created correctly
    if (tunOpen(client, tun_name)) {
        for (int i = 0; i < 100; i++) {
            //addToWriteQueue(client, buff, 10);
            tun_write(client, (payload_t) {.stream = (stream_t*)buff, .len = 10});
        }
    } else {
        printf("not possible to create the tunnel\n");
    }
}
