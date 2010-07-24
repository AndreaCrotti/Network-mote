// TODO: try to put the headers on diet
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
#include <sysexits.h>
#include <stdio.h>

// Functions for using a tunnel device
#include "tunnel.h"

// our own declarations
#include "client.h"

#include "motecomm.h"
#include "chunker.h"

#include "glue.h"
#include "../shared/structs.h"

void startGateway(char const *dev) {
    
}

void usage(char* name) {
    fprintf(stderr, "%s <device>\n",name);
    exit(EX_USAGE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        usage(argv[0]);
    }
    char const* dev = argv[1];
    
    startGateway(dev);
    return 0;
}
