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
char ifname[IFNAMSIZ];

// all possible tun_devices
static tundev tun_devices[MAX_CLIENTS];
static int flags;

char *fetch_from_queue(write_queue *queue);
void add_to_queue(write_queue *queue, char *element);
int is_writable(int fd);
int *get_fd(int client_no);

/** 
 * Setup the flags, basically if using TUN or TAP device
 * 
 * @param tun_flags 
 */
void tunSetup(int tun_flags) {
    flags = tun_flags;
}

/** 
 * Creates a new tun/tap device, or connects to a already existent one depending
 * on the arguments.
 * 
 * @param dev The name of the device to connect to or '\0' when a new device should be
 *            should be created.
 * 
 * @return Error-code.
 */
// TODO: remove the need of *dev also, this should be tun_new and always \0
int tunOpen(int client_no, char *dev) {
    struct ifreq ifr;
    int err;
    char *clonedev = TUN_DEV;
    int *fd = get_fd(client_no);
    
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

    // Set the global ifname variable 
    memcpy(ifname, dev, IFNAMSIZ);

    return *fd;
}

int *get_fd(int client_no) {
    return &(tun_devices[client_no].fd);
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
int tunRead(int client_no, char *buf, int length){
    int fd = *(get_fd(client_no));
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

/** 
 * Add one message to the write queue
 * and then try to send them all out
 * 
 * @param client_no client connected
 * @param buf buffer to send
 * @param len length of the buffer
 */
void addToWriteQueue(int client_no, char *buf, int len) {
    int fd = tun_devices[client_no].fd;
    // add the message to the queue
    write_queue *queue = &(tun_devices[client_no].queue);
    add_to_queue(queue, buf);
    // now use a select to try to send out everything
    
    // try to send out as many messages as possible
    char *message;
    // quit immediately the loop if we sent everything or is not writable
    while (1) {
        message = fetch_from_queue(queue);
        if (!message)
            break;
        
        if (is_writable(fd))
            tun_write(fd, buf, len);
        else
            break;
    }
 }

/** 
 * Check if the device is ready for writing
 * 
 * @param fd file descriptor to check
 */
int is_writable(int fd) {
   fd_set fds;
   struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
   int rc, result;
   FD_ZERO(&fds);
   FD_SET(fd, &fds);
   
   // nds must be greater than any other
   rc = select(fd + 1, NULL, &fds, NULL, &timeout);
   return FD_ISSET(fd,&fds) ? 1 : 0;
}


/**
 * Adding element, managing a full queue with assertion (because it should not happen)
 * 
 * @param queue queue to add to
 * @param element element to add to the queue
 */
void add_to_queue(write_queue *queue, char *element) {
    int pos = (queue->head + 1) % MAX_QUEUED;
    // this mean that the queue is full!
    assert(pos != queue->bottom);
    queue->messages[pos] = element;
    queue->head = pos;
}

/** 
 * @param queue queue
 * 
 * @return the first element inserted into the queue or NULL if not found
 */
char *fetch_from_queue(write_queue *queue) {
    if (queue->head != queue->bottom) {
        queue->bottom--;
        // mm maybe another variable is better here
        return queue->messages[queue->bottom + 1];
    }
    return NULL;
}

#ifdef STANDALONE

// testing the creation and writing/reading from with the tunnel
int main(int argc, char *argv[]) {
    // setup a tun device and then work with it

}

void test_tun_write(void) {
    
}

#endif
