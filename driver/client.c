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

#include "structs.h"
#include "setup.h"

#define CLIENT_NO 0


// Hardcoded sender and destination addresses for the created packets
extern uint16_t sender_address;
extern uint16_t destination_address;

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

    // wrapper for select
    fdglue_t fdg;
    
    mcp_t *mcp;
    serialif_t *sif;
    if (dev) {
        sif = createSerialConnection(dev, &mcp);
    } else {
        sif = createFifoConnection(&mcp);
    }

    initGlue(&fdg,sif,mcp,CLIENT_NO);

    main_loop(&fdg);
}

void usage(char* name) {
    LOG_ERROR("%s [<device>]",name);
    exit(EX_USAGE);
}

int main(int argc, char *argv[]) {
    char const* dev;

    sender_address = 1;
    destination_address = 254;

    if (argc < 2) {
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
