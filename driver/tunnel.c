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

#include "tunnel.h"

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

    return fd;
}

/** 
 * Sets up the tunnel interface and assigns a MTU and a IPv4 address.
 * 
 * @param dev The device name.
 * @param addr An IPv4 address.
 * 
 * @return Error-code.
 */
int tun_setup(char *dev, struct in_addr *addr){
    
    struct ifreq ifr;
    int fd, err;
    int mtu = 1280;
    
    // Getting the device identifier with the socket command
    if( (fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Getting socket");
        return fd;
    }

    // Prepare the ifr struct
    memset(&ifr, 0, sizeof(struct ifreq));
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    // Set up the interface
    ifr.ifr_flags |= IFF_UP;
    if( (err = ioctl(fd, SIOCSIFFLAGS, &ifr)) < 0) {
        perror("Setting up the interface");
        return err;
    }

    // Assign the MTU value
    ifr.ifr_mtu = mtu;
    if( (err = ioctl(fd, SIOCSIFMTU, &ifr)) < 0) {
        perror("Assigning MTU");
        return err;
    }
    
    //TODO: What is SIOGIFINDEX and is it needed??
    
    //TODO: Setting up a bridge

    //TODO: Give an address to the bridge

    //TODO: Set up default routing entry to the bridge

    return 0;
}

/** 
 * Reads data from the tunnel and exits if a error occurred.
 * 
 * @param fd The tunnel device.
 * @param buf This is where the read data are written.
 * @param n maximum number of bytes to read.
 * 
 * @return number of bytes read.
 */
int tun_read(int fd, char *buf, int n){
    
    int nread;

    //TODO: Maybe recv is better here
    if((nread=read(fd, buf, n)) < 0){
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
 * @param n Maximal number of bytes to write.
 * 
 * @return number of bytes written.
 */
int tun_write(int fd, char *buf, int n){
    int nwrite;
    
    //TODO: Maybe send is better here
    if((nwrite=write(fd, buf, n)) < 0){
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}
