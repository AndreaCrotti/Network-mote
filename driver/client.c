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

#include "motecomm.h"
#include "chunker.h"

#include "glue.h"

// a wrapper for mcp::receive that will be understood by the fdglue module
void mcpReceive(fdglue_handler_t* that) {
  mcp_t* this = (mcp_t*)(that->p);
  this->getComm(this)->read(this->getComm(this));
}

#define ETHERNET_FRAME_SIZE 4242

struct TunHandlerInfo {
  int fd;
  ifp_t* ifp;
};

void tunReceive(fdglue_handler_t* that) {
  struct TunHandlerInfo* this = (struct TunHandlerInfo*)(that->p);
  static stream_t buf[ETHERNET_FRAME_SIZE];
  memset(buf,0,ETHERNET_FRAME_SIZE);
  int size = tun_read(this->fd,(char*)buf,ETHERNET_FRAME_SIZE);
  assert(size);
  static int seqno = 0;
  ++seqno;
  payload_t payload = {.stream = buf, .len = size};
  ipv6Packet ipv6;
  while (genIpv6Packet(&payload,&ipv6,seqno)) {
    this->ifp->send(this->ifp,(payload_t){.stream = (stream_t*)&ipv6, .len = ipv6.sendsize});
  }
}

la_t localAddress = {{ 0,0,0,0 }};

void laSet(laep_handler_t* this, la_t const address) {
  (void)this;
  localAddress = address;
}

int main(int args, char** arg) {
    (void)args;
    (void)arg;

    char tun_name[IFNAMSIZ];

    // The IP address
    char *ip_address_str = "10.0.0.1";

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
    (void)buff;

    fdglue_t fdg;
    fdglue(&fdg);
    char mote[] = "telosb";
    char const* dev = arg[1];
    serialif_t* sif = NULL;
    mcp_t* mcp = openMcpConnection(dev,mote,&sif);
    ifp_t _ifp;
    ifp(&_ifp,mcp);
    laep_t _laep;
    laep(&_laep,mcp);
    _laep.setHandler(&_laep,LAEP_REPLY,(laep_handler_t){.handle = laSet, .p = NULL});
    if (!mcp) {
      printf("There was an error opening the connection to %s over device %s.",mote,dev);
    }
    struct TunHandlerInfo thi = {.fd = tun_fd, .ifp = &_ifp};
    fdg.setHandler(&fdg,sif->fd(sif),FDGHT_READ,(fdglue_handler_t){
                                                  .p = mcp,
                                                  .handle = mcpReceive},FDGHR_APPEND);
    fdg.setHandler(&fdg,tun_fd,FDGHT_READ,(fdglue_handler_t){
                                                  .p = &thi,
                                                  .handle = tunReceive},FDGHR_APPEND); //TODO
    for (;;) {
      fdg.listen(&fdg,3600);
    }
    /*
    while(1) {
        memset(buff, 0, size);
        len = tun_read(tun_fd, buff, size);
        if (len > 0) {
            printf("got a message of length %d\n", len);
        } else {
            perror("not receiving anything\n");
        }
    }*/
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

