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

// implementation of a FIFO queue as a circular array
typedef struct write_queue {
    payload_t data[MAX_QUEUED];
    int first;
    int last;
} write_queue;

// structure of a tun device
typedef struct tundev {
    char *ifname;
    int fd;
    int client; // client we're serving
    write_queue queue;
} tundev;


// The name of the interface 
char ifname[IFNAMSIZ];

// all possible tun_devices
static tundev tun_devices[MAX_CLIENTS];
static int flags;

payload_t fetch_from_queue(write_queue *queue);
void add_to_queue(write_queue *queue, payload_t data);
int queue_empty(write_queue *queue);
int queue_full(write_queue *queue);
int is_writable(int fd);
void set_fd(int client_no, int fd);
void delete_last(write_queue *queue);
int tun_write(int fd, payload_t data);


/** 
 * Setup the flags, basically if using TUN or TAP device
 * 
 * @param tun_flags 
 */
void tunSetup(int tun_flags) {
    flags = tun_flags;
    // setup the queue for each of the clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        tun_devices[i].queue.first = 0;
        tun_devices[i].queue.last = 0;
    }
}

// TODO: remove the need of *dev also, this should be tun_new and always \0
// FIXME: should it ever connect to the same device twice?
int tunOpen(int client_no, char *dev) {
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
    if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
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

int getFd(int client_no) {
    return tun_devices[client_no].fd;
}

void set_fd(int client_no, int fd) {
    tun_devices[client_no].fd = fd;
}

int queueEmpty(int client_no) {
    return queue_empty(&(tun_devices[client_no].queue));
}

int tunRead(int client_no, char *buf, int length){
    int fd = getFd(client_no);
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
int tun_write(int fd, payload_t data){
    int nwrite;
    
    // should not exit directly here maybe?
    //TODO: Maybe send is better here
    if((nwrite = write(fd, data.stream, data.len)) < 0){
        perror("Writing data");
        exit(1);
    }
    return nwrite;
}

void tunWriteNoQueue(int client_no, payload_t data) {
    int fd = getFd(client_no);
    // use some simple error checking here instead
    unsigned sent = tun_write(fd, data);
    assert(sent == data.len);
}

void addToWriteQueue(int client_no, payload_t data) {
    assert(0);
    int fd = getFd(client_no);
    // add the message to the queue
    write_queue *queue = &(tun_devices[client_no].queue);
    add_to_queue(queue, data);
    /* printf("now queue %d <-> %d\n", queue->first, queue->last); */
    // now use a select to try to send out everything

    // try to send out as many messages as possible
    payload_t message;
    // quit immediately the loop if we sent everything or is not writable
    while (!queue_empty(queue)) {
        message = fetch_from_queue(queue);
        if (!is_writable(fd)) {
            unsigned nwrite = tun_write(fd, data);
            /* printf("wrote %d bytes\n", nwrite); */
            if (nwrite) {
                // otherwise means partially written data
                assert(nwrite == data.len);
                // only now we can remove it from the queue
                delete_last(queue);
            }
        }
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
    int rc;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    
    // is this fd+1 exactly what we need here?
    rc = select(fd + 1, NULL, &fds, NULL, &timeout);
    return FD_ISSET(fd,&fds) ? 1 : 0;
}

int checkFd(int fd) {
    fd_set fds;
    struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
    int rc;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    
    // is this fd+1 exactly what we need here?
    rc = select(fd + 1, &fds, &fds, &fds, &timeout);
    /* int modes[] = {1, 2, 4}; */
    return FD_ISSET(fd,&fds) ? 1 : 0;
}


/**
 * Adding element, managing a full queue with assertion (because it should not happen)
 * 
 * @param queue queue to add to
 * @param data payload to add to the queue
 */
void add_to_queue(write_queue *queue, payload_t data) {
    assert(!queue_full(queue));
    /* printf("adding %s to the queue\n", element); */
    queue->data[queue->last] = data;
    // going forward of one position
    queue->last = NEXT(queue->last);
}

int queue_full(write_queue *queue) {
    return (NEXT(queue->last) == (queue->first));
}

int queue_empty(write_queue *queue) {
    return (queue->first == queue->last);
}

/** 
 * @param queue queue
 * 
 * @return the first element inserted into the queue or NULL if not found
 */
payload_t fetch_from_queue(write_queue *queue) {
    return queue->data[queue->first];
}

void delete_last(write_queue *queue) {
    queue->first = NEXT(queue->first);
}

