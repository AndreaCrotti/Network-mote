/*******************************************************************************/
/* tunnel.c                                                                    */
/*                                                                             */
/* This file contains a new and simple implementation for the tun/tap drivers  */
/* WITH COMMENTS!                                                              */
/*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <arpa/inet.h>  
#include <net/route.h>
#include <signal.h>

#include "tunnel.h"

// Variables to store a route configuration
char stored_gw = 0;
struct rtentry saved_entry;
// The name of the interface 
char *ifname;

// Storage for our socket identifier
int sock = 0;

/** 
 * Creates a new tun/tap device, or connects to a already existent one depending
 * on the arguments.
 * 
 * @param dev The name of the device to connect to or '\0' when a new device should be
 *            should be created.
 * @param flags IFF_TUN of IFF_TAP, depending on whether tun or tap should be used.
 *              Additionally IFF_NO_PI can be set.
 * 
 * @return Error-code.
 */
int tun_open(char *dev, int flags){
    
    struct ifreq ifr;
    int fd, err;
    char *clonedev = "/dev/net/tun";
    
    // Open the clone device
    if( (fd = open(clonedev , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return fd;
    }
    
    // prepare ifr
    memset(&ifr, 0, sizeof(ifr));
    // Set the flags
    ifr.ifr_flags = flags;

    // If a device name was specified it is put to ifr
    if(*dev){
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // Try to create the device
    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
        close(fd);
        perror("Creating the device");
        return err;
    }

    // Write the name of the new interface to device
    strcpy(dev, ifr.ifr_name);

    // allocate 10 Bytes for the interface name
    ifname = malloc(10);

    return fd;
}

// TODO: remove all the stuff ifconfig-related, will be done in a script instead
/** 
 * This function is used to restore the gateway before the program stops. 
 */
void restore_gateway(int param){
    (void) param;

    int fd, err; 

    // Getting the device identifier with the socket command
    if( (fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Getting socket");
    }

    printf("Restoring the standard route over '%s'\n", saved_entry.rt_dev);

    // Restore the old gateway
    if( (err = ioctl(fd, SIOCADDRT, &saved_entry)) < 0){
        perror("Restoring routing entry");
    }

    exit(0);
}

/** 
 * Sets up the tunnel interface and assigns a MTU and a IPv4 address.
 * 
 * @param dev The device name.
 * @param addr An IPv4 address (As a string).
 * 
 * @return Error-code.
 */
int tun_setup(char *dev, char *addr){
    // TODO: what do we need a socket for when we setup the device??
    // Getting the device identifier with the socket command
    if( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Getting socket");
        return sock;
    }
    return 0;
}

/** 
 * Reads data from the tunnel and exits if a error occurred.
 * 
 * @param fd The tunnel device.
 * @param buf This is where the read data are written.
 * @param length maximum number of bytes to read.
 * 
 * @return number of bytes read.
 */
int tun_read(int fd, char *buf, int length){
    
    int nread;

    if((nread = recv(fd, buf, length, 0)) < 0){
        perror("Reading data");
        exit(1);
    }
    return nread;
}

/** 
 * Writes data from buf to the device.
 * 
 * @param fd The tunnel device.
 * @param buf Pointer to the data.
 * @param length Maximal number of bytes to write.
 * 
 * @return number of bytes written.
 */
int tun_write(int fd, char *buf, int length){
    int nwrite;
    
    //TODO: Maybe send is better here
    if((nwrite=send(fd, buf, length, 0)) < 0){
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}
