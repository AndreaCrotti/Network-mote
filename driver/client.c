// TODO: Refactor IN SMALLER AND NICER FUNCTIONS TO setup.c/h

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
// our own declarations
#include "client.h"

#include "../shared/structs.h"
#include "setup.h"

#define CLIENT_NO 0

void setup_routes(char *tun_name);


int startClient(char const *dev) {
    // on the server instead could create many
    char tun_name[IFNAMSIZ];
    tunSetup(TUNTAP_INTERFACE);

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    
    // it will exit abruptly if it doesn't open it correctly
    tunOpen(CLIENT_NO, tun_name);

    fflush(stdout);

    setup_routes(tun_name);

    // wrapper for select
    fdglue_t fdg;
    fdglue(&fdg);
    
    mcp_t *mcp;
    serialif_t *sif = createSerialConnection(dev, &mcp);

    fflush(stdout);
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

/** 
 * Setting up the routing table, which need iproute2 to work!!
 * 
 */
void setup_routes(char *tun_name) {
    char script_cmd_p[30] = "sh route_setup.sh ";
    char *script_cmd = (char *)malloc(strlen(script_cmd_p) + IFNAMSIZ); 
    script_cmd = strcat(script_cmd_p, tun_name);

    callScript(script_cmd, "tunnel succesfully setup", "routing setting up", 1);
    // FIXME: can I free the memory now??
}
