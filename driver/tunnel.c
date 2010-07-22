/*******************************************************************************/
/* tunnel.c                                                                    */
/*                                                                             */
/* This file contains a new and simple implementation for the tun/tap drivers  */
/* WITH COMMENTS!                                                              */
/*******************************************************************************/
// Probably is not needed to use a socket at all, we can setup everything in a script

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
#include <assert.h>

#include "tunnel.h"
#include "structs.h"

#define TUN_DEV "/dev/net/tun"

// The name of the interface 
char *ifname;

// all possible tun_devices
tundev tun_devices[MAX_CLIENTS];

char *fetch_from_queue(write_queue *queue);
void add_to_queue(write_queue *queue, char *element);


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
// TODO: flags could maybe moved away somewhere?
int tun_open(int client_no, char *dev, int flags) {
    struct ifreq ifr;
    int err;
    char *clonedev = TUN_DEV;
    int *fd = &(tun_devices[client_no].fd);
    
    // Open the clone device
    // FIXME: why do we return the fd even if the open is not correctly done??
    if( (*fd = open(clonedev , O_RDWR)) < 0 ) {
        perror("Opening /dev/net/tun");
        return *fd;
    }
    
    // prepare ifr
    memset(&ifr, 0, sizeof(ifr));
    // Set the flags
    ifr.ifr_flags = flags;

    // If a device name was specified it is put to ifr
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // Try to create the device
    if( (err = ioctl(*fd, TUNSETIFF, (void *) &ifr)) < 0 ){
        close(*fd);
        perror("Creating the device");
        return err;
    }

    // Write the name of the new interface to device
    strcpy(dev, ifr.ifr_name);

    // FIXME: This has to be freed somewhere!
    // allocate 10 Bytes for the interface name
    ifname = malloc(IFNAMSIZ);
    *ifname = *dev;

    return *fd;
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

    if((nread = read(fd, buf, length)) < 0){
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
    if((nwrite=write(fd, buf, length)) < 0){
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}

// every time I add something I try to write out everything
void addToWriteQueue(int client_no, char *buf, int len) {
    int fd = tun_devices[client_no].fd;
    write_queue *queue = &(tun_devices[client_no].queue);
    add_to_queue(queue, buf);
    // now use a select to try to send out everything
    
    char *message;
    do {
        message = fetch_from_queue(queue);
        // TODO: now check with a select if we can write and call
    } while (message);
}

// add one element to the queue
void add_to_queue(write_queue *queue, char *element) {
    int pos = (queue->head + 1) % MAX_QUEUED;
    // this mean that the queue is full!
    assert(pos != queue->bottom);
    queue->messages[pos] = element;
    queue->head = pos;
}

char *fetch_from_queue(write_queue *queue) {
    if (queue->head == queue->bottom)
        return NULL;
    else {
        queue->bottom--;
        // mm maybe another variable is better here
        return queue->messages[queue->bottom + 1];
    }
}
