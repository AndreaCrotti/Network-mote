
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
#define USAGE "%s <usbdevice> <externalInterface>"
#elif SERIAL_STYLE == 1
#define USAGE "%s <host:port> <externalInterface>"
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
#include "structs.h"
#include "setup.h"

#define CLIENT_NO 0
#define TRUE 1

// Hardcoded sender and destination addresses for the created packets
extern uint16_t sender_address;
extern uint16_t destination_address;

/** 
 * Call an external script to setup the iptables rules
 * 
 * @param dev usb device
 * @param eth network interface connected to the internet
 */
void setup_iptables(char const *dev, char const *eth) {
    char script_cmd[100];
    sprintf(script_cmd, "sh gateway.sh %s %s", dev, eth);
    call_script(script_cmd, "setup iptables rules", "setting up routing", 1);
}

/// start the gateway using the serial interface 'sif' and the mcp_t object 'mcp'
/// with the network interface 'eth' (that is for going OUT, not for the serialforwarder!)
void start_gateway(serialif_t* sif, mcp_t* mcp, char const *eth) {
    // on the server instead could create many
    char tun_name[IFNAMSIZ];
    tun_setup(TUNTAP_INTERFACE);

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    
    // it will exit abruptly if it doesn't open it correctly
    tun_open(CLIENT_NO, tun_name);

    setup_iptables(tun_name, eth);

    // wrapper for select
    fdglue_t fdg;

    init_glue(&fdg,sif,mcp,CLIENT_NO);

    main_loop(&fdg);
}

/**
 * Tell the user how to use the client.
 *
 * @param name The program name used by the user.
 */
void usage(char* name) {
    LOG_ERROR(USAGE, name);
    exit(EX_USAGE);
}

/// http://www.geekherocomic.com/2008/08/18/int-mainvoid/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        usage(argv[0]);
    }

    sender_address = 254;
    destination_address = 1;

    char *dev = argv[1];
    char const *eth = argv[2];

    char const fake = dev[0] == '-' && dev[1] == 0;
    LOG_DEBUG("fake: %d",fake);
    
    // creating the serial interface connection
    mcp_t *mcp;
    serialif_t *sif;
#if SERIAL_STYLE == 0
    if (fake) {
        LOG_WARN("Running in stdin/stdout mode. Expecting two different FIFOs (or pipes) to read/write.");
        LOG_INFO("You may run for example:");
        LOG_INFO("mkfifo \"$MYFIFO\" && ./client < \"$MYFIFO\" | ./gateway - eth0 > \"$MYFIFO\"; [ -p \"$MYFIFO\" ] && rm \"$MYFIFO\"");
        sif = create_fifo_connection(&mcp);
    } else {
        sif = create_serial_connection(dev, &mcp);
    }
#elif SERIAL_STYLE == 1
    if (fake) {
        LOG_ERROR("Fake connection ('-') is not supported with the serial forwarding gateway ('%s').",argv[0]);
        exit(1);
    }
    // split host:port into the variables host and port
    char* port = dev;
    while (*port && *port != ':') {
        port++;
    }
    if (*port != ':') {
        LOG_ERROR("You did not supply a port!");
        exit(1);
    }
    *port++ = 0;
    sif = create_sf_connection(dev, port, &mcp);
#else
#error "unsupported serial style"
#endif

    start_gateway(sif,mcp, eth);
    return 0;
}
