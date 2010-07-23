#include <stdlib.h>
#include <stdio.h>

#include <sysexits.h>
#include "client.h"
#include "gateway.h"
#include "string.h"

void usage() {
    fprintf(stderr, "./main [-g | -c] <device>\n");
    exit(EX_USAGE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage();
    }
    char *mode = argv[1];
    char const* dev = argv[2];
    
    if (strcmp(mode, "-g")) {
        printf("starting gateway program\n");
        startGateway(dev);
    }
    if (strcmp(mode, "-c")) {
        printf("starting client program\n");
        startClient(dev);
    }
    return 0;
}
