/**
 * Macro to see if we're working on OSX is __APPLE__
 * TODO: when we have a TAP device it doesn't make sense to create 
 */

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

// Functions for using a tunnel device
#include "tunnel.h"
// For adding an entry to the routing tables
#include "routing.h"
// Include the serial forwarder
//#include <serialsource.h>

// our own declarations
#include "client.h"

int main(int args, char** arg) {
    (void)args;
    (void)arg;

    char tun_name[IFNAMSIZ];
    char input[20];
    (void)input;

    // Variables to pass to the serial forwarder
    /* char* sf_host = "127.0.0.1"; */
    /* int sf_port = 1000; */

    //Variables for a direct serial connection
    /*char *device = "/dev/ttyUSB0";
    (void)device;
    int baud_rate = 115200;
    (void)baud_rate;
    serial_source ser_src;
    (void)ser_src;*/

    // The IP address
    char *ip_address_str = "10.0.0.1";

    // TODO: here we only need to open the device FD and pass it to motecomm program

    // Open serial
    /* int ser_src = open_serial_source(argv[optind], platform_baud_rate(argv[optind + 1]), */
    /*                              1, stderr_msg); */
    /* ser_src = open_serial_source(device, baud_rate, 1, stderr_msg); */
    /* if (!ser_src) { */
    /*   printf("Couldn't open serial port at device %s with baudrate %d\n", device, baud_rate); */
    /*   exit(1); */
    /* } */

    //Connect to mote using serial forwarder
    /* int sf_fd = open_sf_source(sf_host, sf_port); */
    /* if (sf_fd < 0) { */
    /*     printf("Couldn't connect to serial forwarder sf@%s:%d\n", sf_host, sf_port); */
    /*     exit(1); */
    /* } */

    // a new device should be opened!
    tun_name[0] = 0;    
    // create the tap-device
    int tun_fd = tun_open(tun_name, IFF_TAP);
    if (tun_fd < 1) {
        printf("Could not create tunnel device. Fatal.\n");
        return 1;
    } else {
        printf("created tunnel device: %s\n", tun_name);
    }

    // Setup the tunnel (Beneath other things, this sets the ip address)
    if (tun_setup(tun_name, ip_address_str) < 0) {
        printf("configuring the tun failed; aborting\n");
        return 1;
    }
    
    fflush(stdout);

    /* uint8_t buf[sizeof(struct split_ip_msg) + INET_MTU]; */
    /* struct split_ip_msg *msg = (struct split_ip_msg *)buf; */
    int len;
    (void)len;

    int size = 200;
    char *buff = malloc(size);
    while(1) {
        memset(buff, 0, size);
        len = tun_read(tun_fd, buff, size);
        if (len > 0) {
            printf("got a message of length %d\n", len);
        } else {
            perror("not receiving anything\n");
        }
    }
    return 0;
}

/** 
 * Function to print out the contents of an IP packet.
 *
 * Taken from support/sdk/c/blib/driver/serial_tun.c
 * 
 * @param msg 
 */
/* void print_ip_packet(struct split_ip_msg *msg) { */
/*     int i; */
/*     struct generic_header *g_hdr; */
/*     //if (log_getlevel() > LOGLVL_DEBUG) return; */

/*     printf("  nxthdr: 0x%x hlim: 0x%x plen: %i\n", msg->hdr.nxt_hdr, msg->hdr.hlim, ntohs(msg->hdr.plen)); */
/*     printf("  src: "); */
/*     for (i = 0; i < 16; i++) printf("0x%x ", msg->hdr.ip6_src.s6_addr[i]); */
/*     printf("\n"); */
/*     printf("  dst: "); */
/*     for (i = 0; i < 16; i++) printf("0x%x ", msg->hdr.ip6_dst.s6_addr[i]); */
/*     printf("\n"); */

/*     g_hdr = msg->headers; */
/*     while (g_hdr != NULL) { */
/*         printf("header [%i]: ", g_hdr->len); */
/*         for (i = 0; i < g_hdr->len; i++) */
/*             printf("0x%x ", g_hdr->hdr.data[i]); */
/*         printf("\n"); */
/*         g_hdr = g_hdr->next; */
/*     } */

/*     printf("data [%i]:\n\t", msg->data_len); */
/*     for (i = 0; i < msg->data_len; i++) { */
/*         if (i == 0x40) { */
/*             printf (" ...\n"); */
/*             break; */
/*         } */
/*         printf("0x%x ", msg->data[i]); */
/*         if (i % 16 == 15) printf("\n\t"); */
/*         if (i % 16 == 7) printf ("  "); */
/*     } */
/*     printf("\n"); */
/* } */

