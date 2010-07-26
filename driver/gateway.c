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
#include "setup.h"

#define CLIENT_NO 0

void startGateway(char const *dev) {
    // on the server instead could create many
    char tun_name[IFNAMSIZ];
    tunSetup(TUNTAP_INTERFACE);

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    
    // it will exit abruptly if it doesn't open it correctly
    tunOpen(CLIENT_NO, tun_name);

    char script_cmd[100];
    sprintf(script_cmd, "sh gateway.sh %s eth0", tun_name);

    callScript(script_cmd, "setup iptables rules", "routing setting up", 1);

    // wrapper for select
    fdglue_t fdg;
    fdglue(&fdg);
    
    // creating the serial interface connection
    mcp_t *mcp;
    serialif_t *sif = createSerialConnection(dev, &mcp);

    // structures for the handlers, it's an event driven program
    // so we need to setup handlers
    struct TunHandlerInfo thi = {
        .client_no = CLIENT_NO,
        .mcomm = mcp->getComm(mcp)
    };

    fdglue_handler_t hand_sif = {
        .p = mcp,
        .handle = serialReceive
    };
    fdglue_handler_t hand_thi = {
        .p = &thi,
        .handle = tunReceive
    };

    fdg.setHandler(&fdg, sif->fd(sif), FDGHT_READ, hand_sif, FDGHR_APPEND);
    fdg.setHandler(&fdg, getFd(CLIENT_NO), FDGHT_READ, hand_thi, FDGHR_APPEND);

    unsigned lcount = 0;

    for (;;) {
        printf("listening %d ...\n",lcount++);
        fflush(stdout);
        fdg.listen(&fdg, 5 * 60);
    }
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
