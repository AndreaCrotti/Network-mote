#ifndef TUNNEL_H
#define TUNNEL_H

#include "util.h"

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
int tun_open(int client_no, char *dev);

/** 
 * @param client_no 
 * 
 * @return the file descriptor of the client
 */
int get_fd(int client_no);

/** 
 * Close everything gracefully
 * 
 */
void close_all_tunnels();


/** 
 * Reads data from the tunnel and exits if a error occurred.
 * 
 * @param client_no
 * @param buf This is where the read data are written.
 * @param length maximum number of bytes to read.
 * 
 * @return number of bytes read.
 */
int tun_read(int client_no, char *buf, int n);

/** 
 * Write on tun device without using the fancy queue
 * 
 * @param client_no client connected
 * @param data payload to write
 */
void tun_write(int client_no, payload_t data);

/** 
 * Setup the tunnel module
 * 
 * @param tun_flags IFF_TUN or IFF_TAP basically for tun/tap devices
 */
void tun_setup(int tun_flags);

#endif
