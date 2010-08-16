#ifndef CLIENT_H
#define CLIENT_H

// Hardcode the sender and destination addresses for the created packets
#define SENDER_ADDRESS 1
#define DEST_ADDRESS 254

/** 
 * Starts the client program.
 * 
 * @param dev Device descriptor for the USB port of the mote.
 */
void start_client(char const *dev);

#endif
