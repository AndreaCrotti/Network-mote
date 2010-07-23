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

int startClient(char const *dev) {
    // on the server instead could create many
    char tun_name[IFNAMSIZ];
    tunSetup(TUNTAP_INTERFACE);

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    // only in the gateway the first argument should be != 0
    int tun_fd = tunOpen(CLIENT_NO, tun_name);
    if (tun_fd < 1) {
        printf("Could not create tunnel device. Fatal.\n");
        return 1;
    } else {
        printf("created tunnel device: %s\n", tun_name);
    }

    fflush(stdout);

    char script_cmd_p[30] = "sh route_setup.sh ";
    char *script_cmd = (char *)malloc(strlen(script_cmd_p) + IFNAMSIZ); 
    script_cmd = strcat(script_cmd_p, tun_name);

    callScript(script_cmd, "tunnel succesfully setup", "routing setting up", 1);

    fdglue_t fdg;
    fdglue(&fdg);
    char mote[] = "telosb";
    serialif_t* sif = NULL;

    mcp_t* mcp = openMcpConnection(dev, mote, &sif);
    ifp_t _ifp;
    ifp(&_ifp, mcp);
    laep_t _laep;
    laep(&_laep, mcp);
    _laep.setHandler(&_laep,LAEP_REPLY,(laep_handler_t){.handle = laSet, .p = NULL});

    if (mcp) {
        printf("Connection to %s over device %s opened.\n",mote,dev);
    } else {
        printf("There was an error opening the connection to %s over device %s.\n",mote,dev);
    }

    fflush(stdout);
    struct TunHandlerInfo thi = {.fd = tun_fd, .ifp = &_ifp, .mcomm = mcp->getComm(mcp)};
    fdg.setHandler(&fdg, sif->fd(sif), FDGHT_READ, (fdglue_handler_t) {
            .p = mcp,
                .handle = serialReceive},FDGHR_APPEND);
    fdg.setHandler(&fdg,tun_fd,FDGHT_READ,(fdglue_handler_t){
            .p = &thi,
                // it says "warning: initialization from incompatible pointer type"
                .handle = tunReceive},FDGHR_APPEND);

    unsigned lcount = 0;

    for (;;) {
        printf("listening %d ...\n",lcount++);
        fflush(stdout);
        fdg.listen(&fdg, 5 * 60);
    }
}
