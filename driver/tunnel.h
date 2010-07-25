
#ifndef __TUNNEL_H
#define __TUNNEL_H

#include <arpa/inet.h>
#include "util.h"

#define MAX_QUEUED 10


/*************************/
/* Function declarations */
/*************************/

/** 
 * Creates a new tun/tap device, or connects to a already existent one depending
 * on the arguments.
 * 
 * @param dev The name of the device to connect to or '\0' when a new device should be
 *            should be created.
 * 
 * @return Error-code.
 */
int tunOpen(int client_no, char *dev);

/** 
 * Get the fd for one client number
 * 
 * @param client_no 
 * 
 * @return 
 */
int getFd(int client_no);

/** 
 * Reads data from the tunnel and exits if a error occurred.
 * 
 * @param client_no
 * @param buf This is where the read data are written.
 * @param length maximum number of bytes to read.
 * 
 * @return number of bytes read.
 */
int tunRead(int client_no, char *buf, int n);

/**
 * Add one message to the write queue
 * and then try to send them all out
 *
 * @param client_no client connected
 * @param buf buffer to send
 * @param len length of the buffer
 */
void addToWriteQueue(int client_no, payload_t data);

/** 
 * Write on tun device without using the fancy queue
 * 
 * @param client_no client connected
 * @param data payload to write
 */
void tunWriteNoQueue(int client_no, payload_t data);

/**
 * Check if a queue is empty.
 *
 * @param client_no which client?
 */
int queueEmpty(int client_no);

/** 
 * Setup the tunnel module
 * 
 * @param tun_flags IFF_TUN or IFF_TAP basically for tun/tap devices
 */
void tunSetup(int tun_flags);

#endif
