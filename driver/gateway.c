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

// 0: normal usb
// 1: serial forwarding
#ifndef SERIAL_STYLE
#define SERIAL_STYLE 0
#endif

#if SERIAL_STYLE == 0
#define USAGE "./%s <usbdevice> <externalInterface>\n"
#elif SERIAL_STYLE == 1
#define USAGE "./%s <host:port> <externalInterface>\n"
#else
#define USAGE ""
#endif

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
#define TRUE 1


/** 
 * Call an external script to setup the iptables rules
 * 
 * @param dev usb device
 * @param eth network interface connected to the internet
 */
void setup_iptables(char const *dev, char const *eth) {
    char script_cmd[100];
    sprintf(script_cmd, "sh gateway.sh %s %s", dev, eth);
    callScript(script_cmd, "setup iptables rules", "routing setting up", 1);
}

void startGateway(serialif_t* sif, mcp_t* mcp, char const *eth) {
    // on the server instead could create many
    char tun_name[IFNAMSIZ];
    tunSetup(TUNTAP_INTERFACE);

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    
    // it will exit abruptly if it doesn't open it correctly
    tunOpen(CLIENT_NO, tun_name);

    setup_iptables(tun_name, eth);

    // wrapper for select
    fdglue_t fdg;
    fdglue(&fdg);

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
    fprintf(stderr, USAGE, name);
    exit(EX_USAGE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
    }

    char *dev = argv[1];
    char const *eth = argv[2];
    
    // creating the serial interface connection
    mcp_t *mcp;
    serialif_t *sif;
#if SERIAL_STYLE == 0
    sif = createSerialConnection(dev, &mcp);
#elif SERIAL_STYLE == 1
    // split host:port into the variables host and port
    char host[strlen(dev)+1];
    char* port = dev;
    char* h = (char*)host;
    while (*port && *port != ':') {
      *h++ = *port++;
    }
    if (*port != ':') {
      printf("You did not supply a port!");
      exit(1);
    }
    *port++ = 0;
    sif = createSfConnection(host, port, &mcp);
#else
#error "unsupported serial style"
#endif

    startGateway(sif,mcp, eth);
    return 0;
}
