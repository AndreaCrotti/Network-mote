/**
 * Here we encapsulate all the possible functions managing our tunnel
 *
 * There can be many tun devices open at the same time (on the gateway mostly)
 * and we keep them in an array of structures representing each tunnel device.
 *
 * Optionally we can queue the writing on the device with a FIFO queue
 * 
 */

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
#define NEXT(x) ((x + 1) % MAX_QUEUED)

// structure of a tun device
typedef struct tundev {
    char *ifname;
    int fd;
    int client; // client we're serving
} tundev;


// The name of the interface 
char ifname[IFNAMSIZ];

// all possible tun_devices
static tundev tun_devices[MAX_CLIENTS];
static int flags;


int get_fd(int client_no) {
    return tun_devices[client_no].fd;
}

void set_fd(int client_no, int fd) {
    tun_devices[client_no].fd = fd;
}

/** 
 * Setup the flags, basically if using TUN or TAP device
 * 
 * @param tun_flags 
 */
void tun_setup(int tun_flags) {
    flags = tun_flags;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        tun_devices[i].fd = -1;
    }
}

// TODO: remove the need of *dev also, this should be tun_new and always \0
// FIXME: should it ever connect to the same device twice?
int tun_open(int client_no, char *dev) {
    struct ifreq ifr;
    int err;
    char *clonedev = TUN_DEV;
    int fd;
    
    // Open the clone device
    fd = open(clonedev , O_RDWR);
    if (fd < 0) {
        perror("Opening /dev/net/tun");
        exit(1);
    }
    set_fd(client_no, fd);
    
    // prepare ifr
    memset(&ifr, 0, sizeof(ifr));
    // Set the flags
    ifr.ifr_flags = flags;

    // If a device name was specified it is put to ifr
    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    // Try to create the device
    if((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
        close(fd);
        perror("Creating the device");
        return err;
    }

    // Write the name of the new interface to device
    strcpy(dev, ifr.ifr_name);

    // Set the global ifname variable 
    memcpy(ifname, dev, IFNAMSIZ);

    return 1;
}

void close_all_tunnels() {
    int fd;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        fd = tun_devices[i].fd;
        if (fd > 0) {
            close(fd);
        }
    }
}

int tun_read(int client_no, char *buf, int length){
    int fd = get_fd(client_no);
    int nread;

    if((nread = read(fd, buf, length)) < 0){
        perror("Reading data");
        exit(1);
    }
    return nread;
}

void tun_write(int client_no, payload_t data){
    int nwrite;
    int fd = get_fd(client_no);
    
    // should not exit directly here maybe?
    //TODO: Maybe send is better here
    if((nwrite = write(fd, data.stream, data.len)) < 0) {
        perror("Writing data");
        exit(1);
    }
    assert((unsigned) nwrite == data.len);
}
