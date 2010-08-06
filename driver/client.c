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

#include <sysexits.h>

#include "tunnel.h"
// our own declarations
#include "client.h"
#include "reconstruct.h"

#include "../shared/structs.h"
#include "setup.h"

#define CLIENT_NO 0

char notun = 0;

/** 
 * Setting up the routing table, which need iproute2 to work!!
 * 
 */
void setup_routes(char const* const tun_name) {
    char script_cmd[80] = "bash route_setup.sh ";
    strcat(script_cmd, tun_name);
    call_script(script_cmd, "tunnel succesfully set up", "routing setting up", 1);
}

void start_client(char const *dev) {    
    if (!notun) {
        // on the server instead could create many
        char tun_name[IFNAMSIZ];
        tun_setup(TUNTAP_INTERFACE);

        // a new device should be opened!
        tun_name[0] = 0;    
        // create the tap-device

        // it will exit abruptly if it doesn't open it correctly
        tun_open(CLIENT_NO, tun_name);

        fflush(stdout);

        setup_routes(tun_name);
    }

    // wrapper for select
    fdglue_t fdg;
    fdglue(&fdg);
    
    mcp_t *mcp;
    serialif_t *sif;
    if (dev) {
        sif = createSerialConnection(dev, &mcp);
    } else {
        sif = createFifoConnection(&mcp);
    }

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
    if (!notun)
        fdg.setHandler(&fdg, get_fd(CLIENT_NO), FDGHT_READ, hand_thi, FDGHR_APPEND);

    main_loop(&fdg);
}

void usage(char* name) {
    LOG_ERROR("%s [<device>] [notun]",name);
    exit(EX_USAGE);
}

int main(int argc, char *argv[]) {
    notun = (0 == strcmp(argv[argc-1],"notun"));
    
    char const* dev;

    if (argc < 2+notun) {
        LOG_WARN("Running in stdin/stdout mode. Expecting two different FIFOs (or pipes) to read/write.");
        LOG_INFO("You may run for example:");
        LOG_INFO("mkfifo \"$MYFIFO\" && ./client < \"$MYFIFO\" | ./gateway - eth0 > \"$MYFIFO\"; [ -p \"$MYFIFO\" ] && rm \"$MYFIFO\"");
        dev = 0; //special meaning
    } else {
        dev = argv[1];
    }

    start_client(dev);
    return 0;
}
